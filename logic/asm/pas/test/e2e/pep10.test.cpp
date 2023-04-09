#include "pas/driver/pep10.hpp"
#include "builtins/book.hpp"
#include "builtins/figure.hpp"
#include "builtins/registry.hpp"
#include "macro/registry.hpp"
#include "pas/isa/pep10.hpp"
#include "pas/operations/generic/errors.hpp"
#include "pas/operations/pepp/string.hpp"
#include <QObject>
#include <QTest>

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
      QStringList body =
          pas::ops::pepp::formatSource<pas::isa::Pep10ISA>(*root);
      for (auto &line : body)
        qCritical() << line;
      qCritical() << "";

      for (auto &error : pas::ops::generic::collectErrors(*root))
        qCritical() << error.first.value.line << error.second.message;
    }
    QVERIFY(result);
  }

  void standalone_data() {
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
          << fig->typesafeElements()["pep"]->contents << fig->isOS();
    }
  }
};

#include "pep10.test.moc"

QTEST_MAIN(PasE2E_Pep10);
