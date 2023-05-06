#include "pas/driver/pep10.hpp"
#include "builtins/book.hpp"
#include "builtins/figure.hpp"
#include "builtins/registry.hpp"
#include "isa/pep10.hpp"
#include "macro/registry.hpp"
#include "obj/memmap.hpp"
#include "obj/mmio.hpp"
#include "pas/obj/pep10.hpp"
#include "pas/operations/generic/errors.hpp"
#include "pas/operations/pepp/string.hpp"
#include <QObject>
#include <QTest>

static const auto is_diskIn = [](const auto &x) {
  return x.name == "diskIn" && x.direction == obj::IO::Direction::kInput &&
         x.minOffset == 0xFFFC && x.maxOffset == 0xFFFC;
};
static const auto is_charIn = [](const auto &x) {
  return x.name == "charIn" && x.direction == obj::IO::Direction::kInput &&
         x.minOffset == 0xFFFD && x.maxOffset == 0xFFFD;
};
static const auto is_charOut = [](const auto &x) {
  return x.name == "charOut" && x.direction == obj::IO::Direction::kOutput &&
         x.minOffset == 0xFFFE && x.maxOffset == 0xFFFE;
};
static const auto is_pwrOff = [](const auto &x) {
  return x.name == "pwrOff" && x.direction == obj::IO::Direction::kOutput &&
         x.minOffset == 0xFFFF && x.maxOffset == 0xFFFF;
};

class PasE2E_Pep10 : public QObject {
  Q_OBJECT
  QSharedPointer<const builtins::Book> book;
  void loadBookMacros(QSharedPointer<macro::Registry> registry) {
    for (auto &macro : book->macros())
      registry->registerMacro(macro::types::Core, macro);
  }

  QStringList nonunary = {"DECI", "CHARI", "CHARO", "STRO", "DECO"};

  void injectFakeSCallMacros(QSharedPointer<macro::Registry> registry) {
    for (auto &macro : nonunary)
      registry->registerMacro(
          macro::types::Core,
          QSharedPointer<macro::Parsed>::create(
              macro, 2, "LDWT 0,i\nSCALL $1, $2", "pep/10"));
  }

private slots:
  void standalone() {
    QFETCH(QString, chapter);
    QFETCH(QString, figure);
    QFETCH(QString, body);
    QFETCH(bool, isOS);

    // Load macros on each iteration to prevent macros from migrating between
    // tests.
    auto registry = QSharedPointer<macro::Registry>::create();
    loadBookMacros(registry);
    if (!isOS)
      injectFakeSCallMacros(registry);

    auto pipeline = pas::driver::pep10::pipeline(
        {{body, {.isOS = isOS, .ignoreUndefinedSymbols = !isOS}}}, registry);
    auto result = pipeline->assemble(pas::driver::pep10::Stage::End);
    auto target = pipeline->pipelines[0].first;
    QVERIFY(target->bodies.contains(pas::driver::repr::Nodes::name));
    auto root = target->bodies[pas::driver::repr::Nodes::name]
                    .value<pas::driver::repr::Nodes>()
                    .value;
    // print out error messages before failing -- enables debugging broken
    // tests.
    if (!result) {
      QStringList body = pas::ops::pepp::formatSource<isa::Pep10>(*root);
      for (auto &line : body)
        qCritical() << line;
      qCritical() << "";

      for (auto &error : pas::ops::generic::collectErrors(*root))
        qCritical() << error.first.value.line << error.second.message;
    }

    QVERIFY(result);
  }

  void standalone_data() {
    QTest::addColumn<QString>("chapter");
    QTest::addColumn<QString>("figure");
    QTest::addColumn<QString>("body");
    QTest::addColumn<bool>("isOS");
    auto registry = builtins::Registry(nullptr);
    this->book = registry.findBook("Computer Systems, 6th Edition");
    QVERIFY(!book.isNull());

    for (auto &fig : book->figures()) {
      if (!fig->typesafeElements().contains("pep"))
        continue;
      auto chName = fig->chapterName().toStdString();
      auto figName = fig->figureName().toStdString();
      QTest::addRow("Figure %s.%s", chName.data(), figName.data())
          << fig->chapterName() << fig->figureName()
          << fig->typesafeElements()["pep"]->contents << fig->isOS();
    }
  }

  void unified() {
    QFETCH(QString, chapter);
    QFETCH(QString, figure);
    QFETCH(QString, userBody);
    QFETCH(QString, osBody);
    QFETCH(bool, isFullOS);

    // Load macros on each iteration to prevent macros from migrating between
    // tests.
    auto registry = QSharedPointer<macro::Registry>::create();
    loadBookMacros(registry);

    auto pipeline = pas::driver::pep10::pipeline(
        {{osBody, {.isOS = true, .ignoreUndefinedSymbols = false}},
         {userBody, {.isOS = false, .ignoreUndefinedSymbols = false}}},
        registry);
    auto result = pipeline->assemble(pas::driver::pep10::Stage::End);

    // for (auto &x : pipeline->globals->table.keys())
    //   qCritical() << "SYMBOL: " << x;
    QCOMPARE(pipeline->pipelines.size(), 2);

    auto osTarget = pipeline->pipelines[0].first;
    QVERIFY(osTarget->bodies.contains(pas::driver::repr::Nodes::name));
    auto osRoot = osTarget->bodies[pas::driver::repr::Nodes::name]
                      .value<pas::driver::repr::Nodes>()
                      .value;

    auto userTarget = pipeline->pipelines[1].first;
    QVERIFY(userTarget->bodies.contains(pas::driver::repr::Nodes::name));
    auto userRoot = userTarget->bodies[pas::driver::repr::Nodes::name]
                        .value<pas::driver::repr::Nodes>()
                        .value;
    // print out error messages before failing -- enables debugging broken
    // tests.
    if (!result) {
      auto lines = pas::ops::pepp::formatListing<isa::Pep10>(*userRoot);
      for (auto &line : lines)
        qCritical() << line;
      for (auto &error : pas::ops::generic::collectErrors(*osRoot))
        qCritical() << "OS:   " << error.second.message;
      for (auto &error : pas::ops::generic::collectErrors(*userRoot))
        qCritical() << "USER: " << error.second.message;
    }
    auto elf = pas::obj::pep10::createElf();
    pas::obj::pep10::combineSections(*osRoot);
    pas::obj::pep10::writeOS(elf, *osRoot);
    pas::obj::pep10::combineSections(*userRoot);
    pas::obj::pep10::writeUser(elf, *userRoot);
    elf.save(u"%1.%2.elf"_qs.arg(chapter, figure).toStdString());
    QVERIFY(result);

    // Verify MMIO information.
    auto decs = ::obj::getMMIODeclarations(elf);
    QCOMPARE(decs.length(), 4);
    QCOMPARE(std::find_if(decs.cbegin(), decs.cend(), is_diskIn), decs.cend());
    QCOMPARE(std::find_if(decs.cbegin(), decs.cend(), is_charIn), decs.cend());
    QCOMPARE(std::find_if(decs.cbegin(), decs.cend(), is_charOut), decs.cend());
    QCOMPARE(std::find_if(decs.cbegin(), decs.cend(), is_pwrOff), decs.cend());

    auto buf = ::obj::getMMIBuffers(elf);
    QCOMPARE(buf.size(), 1);

    auto memMap = obj::getMemoryMap(elf);
    if (isFullOS) {
      QCOMPARE(memMap.size(), 4);
      // user memory
      memMap[0].seg = 0;
      auto uMem = obj::AddressRegion{
          .r = 1, .w = 1, .x = 1, .minOffset = 0, .maxOffset = 0xfa25};
      QCOMPARE(memMap[0], uMem);
      // System stack
      memMap[1].seg = 0;
      auto ss = obj::AddressRegion{
          .r = 1, .w = 1, .x = 0, .minOffset = 0xfa26, .maxOffset = 0xfaad};
      QCOMPARE(memMap[1], ss);
      // OS text
      memMap[2].seg = 0;
      auto txt = obj::AddressRegion{
          .r = 1, .w = 0, .x = 1, .minOffset = 0xfaae, .maxOffset = 0xfff9};
      QCOMPARE(memMap[2], txt);
      // Carveout for MMIO
      memMap[3].seg = 0;
      auto mmio = obj::AddressRegion{
          .r = 1, .w = 1, .x = 0, .minOffset = 0xfffa, .maxOffset = 0xffff};
      QCOMPARE(memMap[3], mmio);
    }
  }

  void unified_data() {
    QTest::addColumn<QString>("chapter");
    QTest::addColumn<QString>("figure");
    QTest::addColumn<QString>("userBody");
    QTest::addColumn<QString>("osBody");
    QTest::addColumn<bool>("isFullOS");
    auto registry = builtins::Registry(nullptr);
    this->book = registry.findBook("Computer Systems, 6th Edition");
    QVERIFY(!book.isNull());

    for (auto &fig : book->figures()) {
      auto defaultOS = fig->defaultOS();
      if (fig->isOS())
        continue;
      else if (!fig->typesafeElements().contains("pep"))
        continue;
      else if (defaultOS == nullptr)
        continue;
      else if (!defaultOS->typesafeElements().contains("pep"))
        continue;
      auto chName = fig->chapterName().toStdString();
      auto figName = fig->figureName().toStdString();
      QTest::addRow("Figure %s.%s with OS", chName.data(), figName.data())
          << fig->chapterName() << fig->figureName()
          << fig->typesafeElements()["pep"]->contents
          << defaultOS->typesafeElements()["pep"]->contents
          << bool(defaultOS->figureName() == "full");
    }
  }
};

#include "pep10.test.moc"

QTEST_MAIN(PasE2E_Pep10);
