#include <QTest>
#include <QtCore>

#include "bits/operations/swap.hpp"
#include "bits/strings.hpp"
#include "builtins/book.hpp"
#include "builtins/figure.hpp"
#include "builtins/registry.hpp"
#include "macro/registry.hpp"
#include "pas/driver/pep10.hpp"
#include "pas/obj/pep10.hpp"
#include "pas/operations/generic/errors.hpp"
#include "sim/device/broadcast/mmi.hpp"
#include "sim/device/broadcast/mmo.hpp"
#include "sim/device/dense.hpp"
#include "targets/pep10/isa3/cpu.hpp"
#include "targets/pep10/isa3/helpers.hpp"
#include "targets/pep10/isa3/system.hpp"
#include <elfio/elfio.hpp>
static const auto rw =
    sim::api::memory::Operation{.speculative = false,
                                .kind = sim::api::memory::Operation::Kind::data,
                                .effectful = false};

static const auto gs = sim::api::memory::Operation{
    .speculative = false,
    .kind = sim::api::memory::Operation::Kind::data,
    .effectful = false,
};
QSharedPointer<const builtins::Book> book() {
  QString bookName = "Computer Systems, 6th Edition";

  auto reg = builtins::Registry(nullptr);
  auto book = reg.findBook(bookName);
  return book;
}

QSharedPointer<macro::Registry>
registry(QSharedPointer<const builtins::Book> book, QStringList directory) {
  auto macroRegistry = QSharedPointer<::macro::Registry>::create();
  for (auto &macro : book->macros())
    macroRegistry->registerMacro(::macro::types::Core, macro);
  return macroRegistry;
}

struct User {
  QString pep, pepo;
};

void assemble(ELFIO::elfio &elf, QString os, User user,
              QSharedPointer<macro::Registry> reg) {

  QList<QPair<QString, pas::driver::pep10::Features>> targets = {
      {os, {.isOS = true}}};
  if (!user.pep.isEmpty())
    targets.push_back({user.pep, {.isOS = false}});
  auto pipeline = pas::driver::pep10::pipeline(targets, reg);
  auto result = pipeline->assemble(pas::driver::pep10::Stage::End);
  QVERIFY(result);

  auto osTarget = pipeline->pipelines[0].first;
  auto osRoot = osTarget->bodies[pas::driver::repr::Nodes::name]
                    .value<pas::driver::repr::Nodes>()
                    .value;
  QCOMPARE(pas::ops::generic::collectErrors(*osRoot).size(), 0);
  pas::obj::pep10::combineSections(*osRoot);
  pas::obj::pep10::writeOS(elf, *osRoot);

  if (!user.pep.isEmpty()) {
    auto userTarget = pipeline->pipelines[1].first;
    auto userRoot = userTarget->bodies[pas::driver::repr::Nodes::name]
                        .value<pas::driver::repr::Nodes>()
                        .value;
    QCOMPARE(pas::ops::generic::collectErrors(*userRoot).size(), 0);
    pas::obj::pep10::combineSections(*userRoot);
    pas::obj::pep10::writeUser(elf, *userRoot);
  } else {
    auto asStd = user.pepo.toStdString();
    auto bytes = bits::asciiHexToByte({asStd.data(), asStd.size()});
    QVERIFY(bytes);
    pas::obj::pep10::writeUser(elf, *bytes);
  }
}

class Targets_ISA3Pep10_Figures : public QObject {
  Q_OBJECT
private slots:
  void smoke() {
    QFETCH(QString, os);
    QFETCH(QString, userPep);
    QFETCH(QString, userPepo);
    QFETCH(QString, input);
    QFETCH(QByteArray, output);
    QFETCH(bool, isBM);

    // Load book contents, macros.
    auto bookPtr = book();
    auto reg = registry(bookPtr, {});
    auto elf = pas::obj::pep10::createElf();
    assemble(*elf, os, {.pep = userPep, .pepo = userPepo}, reg);

    // Need to reload to properly compute segment addresses.
    elf->save("tmp.elf");
    elf->load("tmp.elf");
    // Skip loading, to save on cycles. However, can't skip dispatch, or
    // main's stack will be wrong.
    auto system = targets::pep10::isa::systemFromElf(*elf, isBM);
    system->init();
    QVERIFY(!system.isNull());
    system->setBootFlags(true, true);
    if (auto charIn = system->input("charIn"); !input.isEmpty() && charIn) {
      auto charInEndpoint = charIn->endpoint();
      for (auto c : input.toStdString())
        charInEndpoint->append_value(c);
    }

    // Run until machine terminates.
    auto pwrOff = system->output("pwrOff");
    auto endpoint = pwrOff->endpoint();
    bool fail = false;
    auto max = 200'000;
    while (system->currentTick() < max && !endpoint->next_value().has_value()) {
      auto ret = system->tick(sim::api::Scheduler::Mode::Jump);
      fail |= ret.second.error != sim::api::tick::Error::Success;
      QVERIFY(!fail);
    }
    QCOMPARE_NE(system->currentTick(), max);
    // TODO: Ensure that pwrOff was written to.
    // Get all charOut values.
    QByteArray actualOut;
    if (auto charOut = system->output("charOut");
        !output.isEmpty() && charOut) {
      auto charOutEndpoint = charOut->endpoint();
      charOutEndpoint->set_to_head();
      for (auto next = charOutEndpoint->next_value(); next.has_value();
           next = charOutEndpoint->next_value())
        actualOut.push_back(*next);
    }
    QCOMPARE(actualOut, output);
  }

  void smoke_data() {
    QTest::addColumn<QString>("os");
    QTest::addColumn<QString>("userPep");
    QTest::addColumn<QString>("userPepo");
    QTest::addColumn<QString>("input");
    QTest::addColumn<QByteArray>("output");
    QTest::addColumn<bool>("isBM");

    auto bookPtr = book();
    auto figures = bookPtr->figures();
    for (auto &figure : figures) {
      if (!figure->typesafeElements().contains("pep") &&
          !figure->typesafeElements().contains("pepo"))
        continue;
      else if (figure->isOS())
        continue;
      QString userPep = "", userPepo = "";
      if (figure->typesafeElements().contains("pep"))
        userPep = figure->typesafeElements()["pep"]->contents;
      else if (figure->typesafeElements().contains("pepo"))
        userPepo = figure->typesafeElements()["pepo"]->contents;
      auto os = figure->defaultOS()->typesafeElements()["pep"]->contents;
      bool isBM = !os.contains("bootFlg");
      auto ch = figure->chapterName(), fig = figure->figureName();
      int num = 0;
      for (auto io : figure->typesafeTests()) {
        auto name = u"Figure %1.%2 on IO %3"_qs.arg(ch).arg(fig).arg(num);
        auto nameAsStd = name.toStdString();
        QString input = io->input.toString();
        QByteArray output = io->output.toString().toUtf8();
        QTest::addRow(nameAsStd.c_str())
            << os << userPep << userPepo << input << output << isBM;
        num++;
      }
    }
  }
};

#include "figs.test.moc"

QTEST_MAIN(Targets_ISA3Pep10_Figures)
