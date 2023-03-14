#include "pas/operations/pepp/assign_addr.hpp"
#include "macro/macro.hpp"
#include "macro/registry.hpp"
#include "pas/ast/generic/attr_address.hpp"
#include "pas/bits/strings.hpp"
#include "pas/driver/pepp.hpp"
#include "pas/isa/pep10.hpp"
#include "pas/operations/pepp/size.hpp"
#include <QObject>
#include <QTest>

using pas::isa::Pep10ISA;
using pas::ops::pepp::Direction;
using pas::ops::pepp::size;
void childRange(QSharedPointer<pas::ast::Node> parent, qsizetype index,
                qsizetype start, qsizetype end) {
  QVERIFY(parent->has<pas::ast::generic::Children>());
  auto children = parent->get<pas::ast::generic::Children>().value;
  QVERIFY(children.size() > index);
  auto child = children[index];
  QVERIFY(child->has<pas::ast::generic::Address>());
  auto address = child->get<pas::ast::generic::Address>().value;
  QCOMPARE(address.start, start % 0xFFFF);
  QCOMPARE(address.end, end % 0xFFFF);
}
class PasOpsPepp_AssignAddress : public QObject {
  Q_OBJECT
private slots:
  void unary() {
    QFETCH(qsizetype, base);
    QString body = "rola\nrolx";
    auto ret = pas::driver::pepp::createParser<Pep10ISA>(false)(body, nullptr);
    QVERIFY(!ret.hadError);
    auto children = ret.root->get<pas::ast::generic::Children>().value;
    QCOMPARE(children.size(), 2);
    pas::ops::pepp::assignAddresses<Pep10ISA>(*ret.root);
    childRange(ret.root, 0, base + 0, base + 0);
    childRange(ret.root, 1, base + 1, base + 1);
  }
  void unary_data() {
    QTest::addColumn<qsizetype>("base");

    QTest::addRow("0") << qsizetype(0);
    // QTest::addRow("200") << qsizetype(200);
    // QTest::addRow("0xFFFE") << qsizetype(0xFFFE);
  }

  void nonunary() {
    QFETCH(qsizetype, base);
    QString body = "adda 0,i\nldwa 0,i";
    auto ret = pas::driver::pepp::createParser<Pep10ISA>(false)(body, nullptr);
    QVERIFY(!ret.hadError);
    auto children = ret.root->get<pas::ast::generic::Children>().value;
    QCOMPARE(children.size(), 2);
    pas::ops::pepp::assignAddresses<Pep10ISA>(*ret.root);
    childRange(ret.root, 0, base + 0, base + 2);
    childRange(ret.root, 1, base + 3, base + 5);
  }
  void nonunary_data() {
    QTest::addColumn<qsizetype>("base");

    QTest::addRow("0") << qsizetype(0);
    // QTest::addRow("200") << qsizetype(200);
    // QTest::addRow("0xFFFE") << qsizetype(0xFFFE);
  }

  void size0Directives() {
    QFETCH(qsizetype, base);
    QFETCH(QString, directive);
    QString body = u"%1 s\n.block 2\n%1 s"_qs.arg(directive);
    auto ret = pas::driver::pepp::createParser<Pep10ISA>(false)(body, nullptr);
    QVERIFY(!ret.hadError);
    auto children = ret.root->get<pas::ast::generic::Children>().value;
    QCOMPARE(children.size(), 3);
    pas::ops::pepp::assignAddresses<Pep10ISA>(*ret.root);
    childRange(ret.root, 0, base + 0, base + 0);
    childRange(ret.root, 1, base + 0, base + 1);
    childRange(ret.root, 2, base + 2, base + 2);
  }
  void size0Directives_data() {
    QTest::addColumn<qsizetype>("base");
    QTest::addColumn<QString>("directive");

    for (auto &str :
         {".IMPORT", ".EXPORT", ".SCALL", ".USCALL", ".INPUT", ".OUTPUT"})
      QTest::addRow("%s", str) << qsizetype(0) << QString::fromStdString(str);
  }

  void ascii() {
    QFETCH(qsizetype, base);
    QFETCH(QString, arg);
    auto len = pas::bits::escapedStringLength(arg);
    QString body = u".block 2\n.ASCII \"%1\""_qs.arg(arg);
    auto ret = pas::driver::pepp::createParser<Pep10ISA>(false)(body, nullptr);
    QVERIFY(!ret.hadError);
    auto children = ret.root->get<pas::ast::generic::Children>().value;
    QCOMPARE(children.size(), 2);
    pas::ops::pepp::assignAddresses<Pep10ISA>(*ret.root);
    childRange(ret.root, 0, base + 0, base + 1);
    childRange(ret.root, 1, base + 2, base + 2 + len - 1);
  }

  void ascii_data() {
    QTest::addColumn<qsizetype>("base");
    QTest::addColumn<QString>("arg");
    QTest::addRow("short string: no escaped") << qsizetype(0) << "hi";
    QTest::addRow("short string: 1 escaped") << qsizetype(0) << ".\\n";
    QTest::addRow("short string: 2 escaped") << qsizetype(0) << "\\r\\n";
    QTest::addRow("short string: 2 hex") << qsizetype(0) << "\\xff\\x00";
    QTest::addRow("long string: no escaped") << qsizetype(0) << "ahi";
    QTest::addRow("long string: 1 escaped") << qsizetype(0) << "a.\\n";
    QTest::addRow("long string: 2 escaped") << qsizetype(0) << "a\\r\\n";
    QTest::addRow("long string: 2 hex") << qsizetype(0) << "a\\xff\\x00";
  }

  void align() {
    QFETCH(qsizetype, align);
    QFETCH(qsizetype, base);
    QString body = u".block 1\n.ALIGN %1\n.block 0"_qs.arg(align);
    auto ret = pas::driver::pepp::createParser<Pep10ISA>(false)(body, nullptr);
    QVERIFY(!ret.hadError);
    auto children = ret.root->get<pas::ast::generic::Children>().value;
    QCOMPARE(children.size(), 3);
    pas::ops::pepp::assignAddresses<Pep10ISA>(*ret.root);
    childRange(ret.root, 0, base + 0, base + 0);
    childRange(ret.root, 1, base + 1,
               (base % align + align) - (base + 1) % align);
    childRange(ret.root, 2, base % align + align, base % align + align);
  }
  void align_data() {
    QTest::addColumn<qsizetype>("align");
    QTest::addColumn<qsizetype>("base");

    // QTest::addRow("ALIGN 1 @ 0") << qsizetype(1) << qsizetype(0);
    QTest::addRow("ALIGN 2 @ 0") << qsizetype(2) << qsizetype(0);
    QTest::addRow("ALIGN 4 @ 0") << qsizetype(4) << qsizetype(0);
    QTest::addRow("ALIGN 8 @ 0") << qsizetype(8) << qsizetype(0);
  }
  void equate() {
    QString body = ".block 1\ns:.EQUATE 10\nn:.EQUATE s";
    auto ret = pas::driver::pepp::createParser<Pep10ISA>(false)(body, nullptr);
    QVERIFY(!ret.hadError);
    auto children = ret.root->get<pas::ast::generic::Children>().value;
    QCOMPARE(children.size(), 3);
    pas::ops::pepp::assignAddresses<Pep10ISA>(*ret.root);
    childRange(ret.root, 0, 0, 0);
    childRange(ret.root, 1, 1, 1);
    childRange(ret.root, 2, 1, 1);
    QVERIFY(children[1]->has<pas::ast::generic::SymbolDeclaration>());
    QCOMPARE(children[1]
                 ->get<pas::ast::generic::SymbolDeclaration>()
                 .value->value->value()(),
             10);
    QCOMPARE(children[2]
                 ->get<pas::ast::generic::SymbolDeclaration>()
                 .value->value->value()(),
             10);
  }
  // Don't test macros, they shouldn't survive as nodes into the address
  // assignment stage.
  // Empty lines do not matter.
};

#include "assign_addr.test.moc"

QTEST_MAIN(PasOpsPepp_AssignAddress)
