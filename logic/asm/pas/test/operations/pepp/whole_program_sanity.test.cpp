#include "pas/operations/pepp/whole_program_sanity.hpp"
#include "macro/registry.hpp"
#include "pas/driver/pep10.hpp"
#include "pas/driver/pepp.hpp"
#include "pas/errors.hpp"
#include "pas/isa/pep10.hpp"
#include "pas/operations/pepp/assign_addr.hpp"
#include <QObject>
#include <QTest>

namespace E = pas::errors::pepp;
class PasOpsPepp_WholeProgramSanity : public QObject {
  Q_OBJECT
private slots:
  void smoke() {
    QFETCH(QString, source);
    QFETCH(QStringList, errors);
    QFETCH(bool, useDriver);
    QFETCH(bool, useOSFeats);

    QSharedPointer<pas::ast::Node> root;
    if (useDriver) {
      auto pipeline =
          pas::driver::pep10::stages(source, {.isOS = useOSFeats});
      auto pipelines = pas::driver::Pipeline<pas::driver::pep10::Stage>{};
      pipelines.pipelines.push_back(pipeline);
      pipelines.globals = QSharedPointer<pas::driver::Globals>::create();
      pipelines.globals->macroRegistry =
          QSharedPointer<macro::Registry>::create();
      QCOMPARE(
          pipelines.assemble(pas::driver::pep10::Stage::WholeProgramSanity),
          errors.size() == 0);
      QCOMPARE(pipelines.pipelines[0].first->stage,
               errors.size() == 0
                   ? pas::driver::pep10::Stage::End
                   : pas::driver::pep10::Stage::WholeProgramSanity);
      QVERIFY(pipelines.pipelines[0].first->bodies.contains(
          pas::driver::repr::Nodes::name));
      root = pipelines.pipelines[0]
                 .first->bodies[pas::driver::repr::Nodes::name]
                 .value<pas::driver::repr::Nodes>()
                 .value;
    } else {
      auto parseRoot =
          pas::driver::pepp::createParser<pas::isa::Pep10ISA>(false);
      auto res = parseRoot(source, nullptr);
      QVERIFY(!res.hadError);
      pas::ops::pepp::assignAddresses<pas::isa::Pep10ISA>(*res.root);
      root = res.root;
      QCOMPARE(pas::ops::pepp::checkWholeProgramSanity<pas::isa::Pep10ISA>(
                   *root, {.allowOSFeatures = useOSFeats}),
               errors.size() == 0);
    }
    auto actualErrors = pas::ops::generic::collectErrors(*root);
    QCOMPARE(actualErrors.size(), errors.size());
    for (int it = 0; it < actualErrors.size(); it++)
      QCOMPARE(actualErrors[it].second.message, errors[it]);
  }

  void smoke_data() {
    QTest::addColumn<QString>("source");
    QTest::addColumn<QStringList>("errors");
    QTest::addColumn<bool>("useDriver");
    QTest::addColumn<bool>("useOSFeats");

    QTest::addRow("noBurn: visitor")
        << u".BURN 0xFFFF\n.BLOCK 1\n"_qs
        << QStringList{E::illegalDirective.arg(".BURN")} << false << false;
    QTest::addRow("noBurn: driver")
        << u".BURN 0xFFFF\n.BLOCK 1\n"_qs
        << QStringList{E::illegalDirective.arg(".BURN")} << true << false;

    QTest::addRow("size0xFFFF: visitor")
        << u".BLOCK 0xFFFF\n"_qs << QStringList{} << false << false;
    QTest::addRow("size0xFFFF: driver")
        << u".BLOCK 0xFFFF\n"_qs << QStringList{} << true << false;

    QTest::addRow("size0x10000: visitor")
        << u".BLOCK 0xFFFF\n.block 2"_qs << QStringList{E::objTooBig} << false
        << false;
    QTest::addRow("size0x10000: driver")
        << u".BLOCK 0xFFFF\n.block 2"_qs << QStringList{E::objTooBig} << true
        << false;

    for (auto &str :
         {".IMPORT", ".EXPORT", ".SCALL", ".USCALL", ".INPUT", ".OUTPUT"}) {
      QString source = u"%1 s\ns:.block 1\n"_qs.arg(str);
      QTest::addRow("%s in user: visitor", str)
          << source << QStringList{E::illegalInUser.arg(str)} << false << false;
      QTest::addRow("%s in user: driver", str)
          << source << QStringList{E::illegalInUser.arg(str)} << true << false;
      QTest::addRow("%s in OS: visitor", str)
          << source << QStringList{} << false << true;
      QTest::addRow("%s in OS: driver", str)
          << source << QStringList{} << true << true;
    }

    QTest::addRow("no END: visitor")
        << u".BLOCK 0xFFFF\n.END"_qs
        << QStringList{E::illegalDirective.arg(".END")} << false << false;
    QTest::addRow("no END: driver")
        << u".BLOCK 0xFFFF\n.END"_qs
        << QStringList{E::illegalDirective.arg(".END")} << true << false;

    QTest::addRow("no undefined args: visitor")
        << u"LDWA s,i\n"_qs << QStringList{E::undefinedSymbol.arg("s")} << false
        << false;
    QTest::addRow("no undefined args: driver")
        << u"LDWA s,i\n"_qs << QStringList{E::undefinedSymbol.arg("s")} << true
        << false;

    QTest::addRow("no multiply defined symbols: visitor")
        << u"s:.BLOCK 2\ns:.block 2\n"_qs
        << QStringList{E::multiplyDefinedSymbol.arg("s"),
                       E::multiplyDefinedSymbol.arg("s")}
        << false << false;
    QTest::addRow("no multiply defined symbols: driver")
        << u"s:.BLOCK 2\ns:.block 2\n"_qs
        << QStringList{E::multiplyDefinedSymbol.arg("s"),
                       E::multiplyDefinedSymbol.arg("s")}
        << true << false;
  }

  /*void requireEnd() {
    QString source = ".BLOCK 2";
    auto parsed = pas::driver::pepp::createParser<pas::isa::Pep10ISA>(false)(
        source, nullptr);
    pas::ops::pepp::assignAddresses<pas::isa::Pep10ISA>(*parsed.root);
    QVERIFY(!pas::ops::pepp::checkWholeProgramSanity<pas::isa::Pep10ISA>(
        *parsed.root, {.allowOSFeatures = false}));
    auto errors = pas::ops::generic::collectErrors(*parsed.root);
    QCOMPARE(errors.size(), 1);
    QCOMPARE(errors[0].second.message, E::missingEnd);
  }*/
};

#include "whole_program_sanity.test.moc"

QTEST_MAIN(PasOpsPepp_WholeProgramSanity)
