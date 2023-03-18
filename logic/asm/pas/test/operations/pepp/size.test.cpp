#include "pas/operations/pepp/size.hpp"
#include "macro/macro.hpp"
#include "macro/registry.hpp"
#include "pas/bits/strings.hpp"
#include "pas/driver/pepp.hpp"
#include "pas/isa/pep10.hpp"
#include <QObject>
#include <QTest>

using pas::isa::Pep10ISA;
using pas::ops::pepp::Direction;
using pas::ops::pepp::explicitSize;
class PasOpsPepp_Size : public QObject {
  Q_OBJECT
private slots:
  void unary() {
    QString body = "rola\nrolx";
    auto ret = pas::driver::pepp::createParser<Pep10ISA>(false)(body, nullptr);
    QVERIFY(!ret.hadError);
    auto children = ret.root->get<pas::ast::generic::Children>().value;
    QCOMPARE(children.size(), 2);
    for (auto &base : {0, 200, 0xfffe}) {
      QCOMPARE(explicitSize<Pep10ISA>(*ret.root, base, Direction::Forward), 2);
      QCOMPARE(explicitSize<Pep10ISA>(*ret.root, base, Direction::Backward), 2);
    }
  }
  void nonUnary() {
    QString body = "ldwa n,x\nstwa n,x";
    auto ret = pas::driver::pepp::createParser<Pep10ISA>(false)(body, nullptr);
    QVERIFY(!ret.hadError);
    auto children = ret.root->get<pas::ast::generic::Children>().value;
    QCOMPARE(children.size(), 2);
    for (auto &base : {0, 200, 0xfffe}) {
      QCOMPARE(explicitSize<Pep10ISA>(*ret.root, base, Direction::Forward), 6);
      QCOMPARE(explicitSize<Pep10ISA>(*ret.root, base, Direction::Backward), 6);
    }
  }
  void size0Directives() {
    QFETCH(QString, body);
    auto ret = pas::driver::pepp::createParser<Pep10ISA>(false)(
        u"%1 s"_qs.arg(body), nullptr);
    QVERIFY(!ret.hadError);
    auto children = ret.root->get<pas::ast::generic::Children>().value;
    QCOMPARE(children.size(), 1);
    for (auto &base : {0, 200, 0xfffe}) {
      QCOMPARE(explicitSize<Pep10ISA>(*ret.root, base, Direction::Forward), 0);
      QCOMPARE(explicitSize<Pep10ISA>(*ret.root, base, Direction::Backward), 0);
    }
  }
  void size0Directives_data() {
    QTest::addColumn<QString>("body");
    for (auto &str :
         {".IMPORT", ".EXPORT", ".SCALL", ".USCALL", ".INPUT", ".OUTPUT"}) {
      QTest::addRow(str) << QString::fromStdString(str);
    }
  }
  void ascii() {
    QFETCH(QString, arg);
    auto ret = pas::driver::pepp::createParser<Pep10ISA>(false)(
        u".ASCII \"%1\""_qs.arg(arg), nullptr);
    QVERIFY(!ret.hadError);
    auto children = ret.root->get<pas::ast::generic::Children>().value;
    QCOMPARE(children.size(), 1);
    auto len = pas::bits::escapedStringLength(arg);
    for (auto &base : {0, 200, 0xfffe}) {
      QCOMPARE(explicitSize<Pep10ISA>(*ret.root, base, Direction::Forward),
               len);
      QCOMPARE(explicitSize<Pep10ISA>(*ret.root, base, Direction::Backward),
               len);
    }
  }

  void ascii_data() {
    QTest::addColumn<QString>("arg");
    QTest::addRow("short string: no escaped") << "hi";
    QTest::addRow("short string: 1 escaped") << ".\\n";
    QTest::addRow("short string: 2 escaped") << "\\r\\n";
    QTest::addRow("short string: 2 hex") << "\\xff\\x00";
    QTest::addRow("long string: no escaped") << "ahi";
    QTest::addRow("long string: 1 escaped") << "a.\\n";
    QTest::addRow("long string: 2 escaped") << "a\\r\\n";
    QTest::addRow("long string: 2 hex") << "a\\xff\\x00";
  }

  void align() {
    QFETCH(qsizetype, align);
    QFETCH(qsizetype, base);
    auto ret = pas::driver::pepp::createParser<Pep10ISA>(false)(
        u".ALIGN %1"_qs.arg(align), nullptr);
    QVERIFY(!ret.hadError);
    auto children = ret.root->get<pas::ast::generic::Children>().value;
    QCOMPARE(children.size(), 1);
    auto forwardToNextAlign = (align - (base % align)) % align;
    auto forwardEnd = base + forwardToNextAlign;
    auto forwardSize = forwardEnd - base;
    QCOMPARE(explicitSize<Pep10ISA>(*ret.root, base, Direction::Forward),
             forwardSize);
    auto backwardStart = base - (base % align);
    auto backwardSize = base - backwardStart;
    QCOMPARE(explicitSize<Pep10ISA>(*ret.root, base, Direction::Backward),
             backwardSize);
  }
  void align_data() {
    QTest::addColumn<qsizetype>("align");
    QTest::addColumn<qsizetype>("base");

    QTest::addRow("ALIGN 1 @ 0") << qsizetype(1) << qsizetype(0);
    QTest::addRow("ALIGN 1 @ 1") << qsizetype(1) << qsizetype(1);
    QTest::addRow("ALIGN 1 @ 2") << qsizetype(1) << qsizetype(2);
    QTest::addRow("ALIGN 1 @ FFFE") << qsizetype(1) << qsizetype(0xfffe);
    QTest::addRow("ALIGN 2 @ 0") << qsizetype(2) << qsizetype(0);
    QTest::addRow("ALIGN 2 @ 1") << qsizetype(2) << qsizetype(1);
    QTest::addRow("ALIGN 2 @ 2") << qsizetype(2) << qsizetype(2);
    QTest::addRow("ALIGN 2 @ FFFE") << qsizetype(2) << qsizetype(0xfffe);
    QTest::addRow("ALIGN 4 @ 0") << qsizetype(4) << qsizetype(0);
    QTest::addRow("ALIGN 4 @ 1") << qsizetype(4) << qsizetype(1);
    QTest::addRow("ALIGN 4 @ 2") << qsizetype(4) << qsizetype(2);
    QTest::addRow("ALIGN 4 @ FFFE") << qsizetype(4) << qsizetype(0xfffe);
    QTest::addRow("ALIGN 8 @ 0") << qsizetype(8) << qsizetype(0);
    QTest::addRow("ALIGN 8 @ 1") << qsizetype(8) << qsizetype(1);
    QTest::addRow("ALIGN 8 @ 2") << qsizetype(8) << qsizetype(2);
    QTest::addRow("ALIGN 8 @ FFFE") << qsizetype(8) << qsizetype(0xfffe);
  }

  void block() {
    QFETCH(qsizetype, count);
    auto ret = pas::driver::pepp::createParser<Pep10ISA>(false)(
        u".BLOCK %1"_qs.arg(count), nullptr);
    QVERIFY(!ret.hadError);
    auto children = ret.root->get<pas::ast::generic::Children>().value;
    QCOMPARE(children.size(), 1);
    for (auto &base : {0, 200, 0xfffe}) {
      QCOMPARE(explicitSize<Pep10ISA>(*ret.root, base, Direction::Forward),
               count);
      QCOMPARE(explicitSize<Pep10ISA>(*ret.root, base, Direction::Backward),
               count);
    }
  }

  void block_data() {
    QTest::addColumn<qsizetype>("count");
    QTest::addRow("0") << qsizetype(0);
    QTest::addRow("FFFF") << qsizetype(0xFFFF);
    QTest::addRow("1") << qsizetype(1);
    QTest::addRow("16") << qsizetype(0x10);
  }

  void wordSize() {
    auto ret = pas::driver::pepp::createParser<Pep10ISA>(false)(
        u".WORD 0\n .WORD 1\n.WORD 0xFFFF"_qs, nullptr);
    QVERIFY(!ret.hadError);
    auto children = ret.root->get<pas::ast::generic::Children>().value;
    QCOMPARE(children.size(), 3);
    for (auto &base : {0, 200, 0xfffe}) {
      QCOMPARE(explicitSize<Pep10ISA>(*ret.root, base, Direction::Forward), 6);
      QCOMPARE(explicitSize<Pep10ISA>(*ret.root, base, Direction::Backward), 6);
    }
  }

  void byteSize() {
    auto ret = pas::driver::pepp::createParser<Pep10ISA>(false)(
        u".BYTE 0\n .BYTE 1\n.BYTE 0xFF"_qs, nullptr);
    QVERIFY(!ret.hadError);
    auto children = ret.root->get<pas::ast::generic::Children>().value;
    QCOMPARE(children.size(), 3);
    for (auto &base : {0, 200, 0xfffe}) {
      QCOMPARE(explicitSize<Pep10ISA>(*ret.root, base, Direction::Forward), 3);
      QCOMPARE(explicitSize<Pep10ISA>(*ret.root, base, Direction::Backward), 3);
    }
  }

  void macro() {
    auto registry = QSharedPointer<macro::Registry>::create();
    auto macro = QSharedPointer<macro::Parsed>::create(
        u"alpha"_qs, 0, u".BYTE 1\n.WORD 2"_qs, u"pep/10"_qs);
    registry->registerMacro(macro::types::Core, macro);
    auto parseRoot = pas::driver::pepp::createParser<Pep10ISA>(false);
    auto res = parseRoot(u"@alpha\n.END"_qs, nullptr);
    QVERIFY(!res.hadError);
    auto ret = pas::ops::generic::includeMacros(
        *res.root, pas::driver::pepp::createParser<Pep10ISA>(true), registry);
    QVERIFY(ret);
    QCOMPARE(explicitSize<Pep10ISA>(*res.root, 0, Direction::Forward), 3);
    QCOMPARE(explicitSize<Pep10ISA>(*res.root, 0, Direction::Backward), 3);
  }

  void empty() {
    auto ret =
        pas::driver::pepp::createParser<Pep10ISA>(false)(u"\n\n\n"_qs, nullptr);
    QVERIFY(!ret.hadError);
    auto children = ret.root->get<pas::ast::generic::Children>().value;
    QCOMPARE(children.size(), 4);
    for (auto &base : {0, 200, 0xfffe}) {
      QCOMPARE(explicitSize<Pep10ISA>(*ret.root, base, Direction::Forward), 0);
      QCOMPARE(explicitSize<Pep10ISA>(*ret.root, base, Direction::Backward), 0);
    }
  }
};

#include "size.test.moc"

QTEST_MAIN(PasOpsPepp_Size);
