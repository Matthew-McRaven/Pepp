#include "pas/operations/pepp/size.hpp"
#include "pas/bits/strings.hpp"
#include "pas/driver/pepp.hpp"
#include "pas/isa/pep10.hpp"
#include <QObject>
#include <QTest>

using pas::isa::Pep10ISA;
using pas::ops::pepp::Direction;
using pas::ops::pepp::size;
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
      QCOMPARE(size<Pep10ISA>(*ret.root, base, Direction::Forward), 2);
      QCOMPARE(size<Pep10ISA>(*ret.root, base, Direction::Backward), 2);
    }
  }
  void nonUnary() {
    QString body = "ldwa n,x\nstwa n,x";
    auto ret = pas::driver::pepp::createParser<Pep10ISA>(false)(body, nullptr);
    QVERIFY(!ret.hadError);
    auto children = ret.root->get<pas::ast::generic::Children>().value;
    QCOMPARE(children.size(), 2);
    for (auto &base : {0, 200, 0xfffe}) {
      QCOMPARE(size<Pep10ISA>(*ret.root, base, Direction::Forward), 6);
      QCOMPARE(size<Pep10ISA>(*ret.root, base, Direction::Backward), 6);
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
      QCOMPARE(size<Pep10ISA>(*ret.root, base, Direction::Forward), 0);
      QCOMPARE(size<Pep10ISA>(*ret.root, base, Direction::Backward), 0);
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
      QCOMPARE(size<Pep10ISA>(*ret.root, base, Direction::Forward), len);
      QCOMPARE(size<Pep10ISA>(*ret.root, base, Direction::Backward), len);
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

  void align() {}
  void align_data() {}
  void block() {}
  void sizeFixedDirectives() {}
  void macro() {}
  void empty() {}
};

#include "size.test.moc"

QTEST_MAIN(PasOpsPepp_Size);
