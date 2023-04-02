#include "pas/operations/pepp/whole_program_sanity.hpp"
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
  void noBurn() {
    QString source = ".BURN 0xFFFF\n.BLOCK 1\n.END";
    auto parsed = pas::driver::pepp::createParser<pas::isa::Pep10ISA>(false)(
        source, nullptr);
    pas::ops::pepp::assignAddresses<pas::isa::Pep10ISA>(*parsed.root);
    QVERIFY(!pas::ops::pepp::checkWholeProgramSanity<pas::isa::Pep10ISA>(
        *parsed.root, {.allowOSFeatures = false}));
    auto errors = pas::ops::generic::collectErrors(*parsed.root);
    QCOMPARE(errors.size(), 1);
    QCOMPARE(errors[0].second.message, E::illegalDirective.arg(".BURN"));
  }
  void size0xFFFF() {
    QString source = ".BLOCK 0xFFFF\n.END";
    auto parsed = pas::driver::pepp::createParser<pas::isa::Pep10ISA>(false)(
        source, nullptr);
    pas::ops::pepp::assignAddresses<pas::isa::Pep10ISA>(*parsed.root);
    QVERIFY(pas::ops::pepp::checkWholeProgramSanity<pas::isa::Pep10ISA>(
        *parsed.root, {.allowOSFeatures = false}));
  }
  void size0x10000() {
    QString source = ".BLOCK 0xFFFF\n.BLOCK 2\n.END";
    auto parsed = pas::driver::pepp::createParser<pas::isa::Pep10ISA>(false)(
        source, nullptr);
    pas::ops::pepp::assignAddresses<pas::isa::Pep10ISA>(*parsed.root);
    QVERIFY(!pas::ops::pepp::checkWholeProgramSanity<pas::isa::Pep10ISA>(
        *parsed.root, {.allowOSFeatures = false}));
    auto errors = pas::ops::generic::collectErrors(*parsed.root);
    QCOMPARE(errors.size(), 1);
    QCOMPARE(errors[0].second.message, E::objTooBig);
  }
  void noOSFeatures() {
    QFETCH(QString, op);
    QString source = u".%1 s\n.END"_qs.arg(op);
    auto parsed = pas::driver::pepp::createParser<pas::isa::Pep10ISA>(false)(
        source, nullptr);
    pas::ops::pepp::assignAddresses<pas::isa::Pep10ISA>(*parsed.root);
    QVERIFY(pas::ops::pepp::checkWholeProgramSanity<pas::isa::Pep10ISA>(
        *parsed.root, {.allowOSFeatures = true}));
    QVERIFY(!pas::ops::pepp::checkWholeProgramSanity<pas::isa::Pep10ISA>(
        *parsed.root, {.allowOSFeatures = false}));
    auto errors = pas::ops::generic::collectErrors(*parsed.root);
    QCOMPARE(errors.size(), 1);
    QCOMPARE(errors[0].second.message, E::illegalInUser.arg("." + op));
  }
  void noOSFeatures_data() {
    QTest::addColumn<QString>("op");

    QTest::addRow("IMPORT") << "IMPORT";
    QTest::addRow("EXPORT") << "EXPORT";
    QTest::addRow("INPUT") << "INPUT";
    QTest::addRow("OUTPUT") << "OUTPUT";
    QTest::addRow("USCALL") << "USCALL";
    QTest::addRow("SCALL") << "SCALL";
  }

  void requireEnd() {
    QString source = ".BLOCK 2";
    auto parsed = pas::driver::pepp::createParser<pas::isa::Pep10ISA>(false)(
        source, nullptr);
    pas::ops::pepp::assignAddresses<pas::isa::Pep10ISA>(*parsed.root);
    QVERIFY(!pas::ops::pepp::checkWholeProgramSanity<pas::isa::Pep10ISA>(
        *parsed.root, {.allowOSFeatures = false}));
    auto errors = pas::ops::generic::collectErrors(*parsed.root);
    QCOMPARE(errors.size(), 1);
    QCOMPARE(errors[0].second.message, E::missingEnd);
  }
};

#include "whole_program_sanity.test.moc"

QTEST_MAIN(PasOpsPepp_WholeProgramSanity)
