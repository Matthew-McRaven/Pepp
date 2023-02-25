#include <QObject>
#include <QTest>

#include "pas/isa/pep10.hpp"
#include "pas/operations/generic/is.hpp"
#include "pas/operations/pepp/is.hpp"
#include "pas/operations/pepp/node_from_parse_tree.hpp"
#include "pas/parse/pepp/rules_lines.hpp"

// Declare all matchers as globals, so they don't fall out of scope in data fn.
// Must manually type erase, since QTest can;'t figure this out on its own.

QSharedPointer<pas::ops::ConstOp<bool>> isBlank =
    QSharedPointer<pas::ops::generic::isBlank>::create();
QSharedPointer<pas::ops::ConstOp<bool>> isComment =
    QSharedPointer<pas::ops::generic::isComment>::create();

// Instructions
QSharedPointer<pas::ops::ConstOp<bool>> isUType =
    QSharedPointer<pas::ops::pepp::isUType<pas::isa::Pep10ISA>>::create();
QSharedPointer<pas::ops::ConstOp<bool>> isRType =
    QSharedPointer<pas::ops::pepp::isRType<pas::isa::Pep10ISA>>::create();
QSharedPointer<pas::ops::ConstOp<bool>> isAType =
    QSharedPointer<pas::ops::pepp::isAType<pas::isa::Pep10ISA>>::create();
QSharedPointer<pas::ops::ConstOp<bool>> isAAAType =
    QSharedPointer<pas::ops::pepp::isAAAType<pas::isa::Pep10ISA>>::create();
QSharedPointer<pas::ops::ConstOp<bool>> isRAAAType =
    QSharedPointer<pas::ops::pepp::isRAAAType<pas::isa::Pep10ISA>>::create();

// Directives
QSharedPointer<pas::ops::ConstOp<bool>> isAlign =
    QSharedPointer<pas::ops::generic::isAlign>::create();
QSharedPointer<pas::ops::ConstOp<bool>> isASCII = []() {
  auto ret = QSharedPointer<pas::ops::generic::isString>::create();
  ret->directiveAliases = {u"ASCII"_qs};
  return ret;
}();
QSharedPointer<pas::ops::ConstOp<bool>> isBlock = []() {
  auto ret = QSharedPointer<pas::ops::generic::isSkip>::create();
  ret->allowFill = false;
  ret->directiveAliases = {u"BLOCK"_qs};
  return ret;
}();
QSharedPointer<pas::ops::ConstOp<bool>> isBurn =
    QSharedPointer<pas::ops::pepp::isBurn>::create();
QSharedPointer<pas::ops::ConstOp<bool>> isByte = []() {
  auto ret = QSharedPointer<pas::ops::generic::isByte1>::create();
  ret->allowMultiple = false;
  ret->directiveAliases = {u"BYTE"_qs};
  return ret;
}();
QSharedPointer<pas::ops::ConstOp<bool>> isEnd =
    QSharedPointer<pas::ops::pepp::isEnd>::create();
QSharedPointer<pas::ops::ConstOp<bool>> isEquate = []() {
  auto ret = QSharedPointer<pas::ops::generic::isSet>::create();
  ret->directiveAliases = {u"EQUATE"_qs};
  return ret;
}();
QSharedPointer<pas::ops::ConstOp<bool>> isExport =
    QSharedPointer<pas::ops::pepp::isExport>::create();
QSharedPointer<pas::ops::ConstOp<bool>> isImport =
    QSharedPointer<pas::ops::pepp::isImport>::create();
QSharedPointer<pas::ops::ConstOp<bool>> isInput =
    QSharedPointer<pas::ops::pepp::isInput>::create();
QSharedPointer<pas::ops::ConstOp<bool>> isOutput =
    QSharedPointer<pas::ops::pepp::isOutput>::create();
QSharedPointer<pas::ops::ConstOp<bool>> isSCall =
    QSharedPointer<pas::ops::pepp::isSCall>::create();
QSharedPointer<pas::ops::ConstOp<bool>> isSection =
    QSharedPointer<pas::ops::pepp::isSection>::create();
QSharedPointer<pas::ops::ConstOp<bool>> isUSCall =
    QSharedPointer<pas::ops::pepp::isUSCall>::create();
QSharedPointer<pas::ops::ConstOp<bool>> isWord = []() {
  auto ret = QSharedPointer<pas::ops::generic::isByte2>::create();
  ret->allowMultiple = false;
  ret->directiveAliases = {u"WORD"_qs};
  return ret;
}();

using pas::ast::Node;
class PasOpsPepp_NodeFromParseTree : public QObject {
  Q_OBJECT
private slots:
  void testVisitor() {
    QFETCH(QString, input);
    QFETCH(QSharedPointer<pas::ops::ConstOp<bool>>, fn);
    auto asStd = input.toStdString();
    using namespace pas::parse::pepp;
    std::vector<LineType> result;
    bool success = true;
    QVERIFY_THROWS_NO_EXCEPTION(
        [&]() { success = parse(asStd.begin(), asStd.end(), line, result); }());
    QVERIFY(success);
    QCOMPARE(result.size(), 1);
    auto visit = pas::operations::pepp::FromParseTree<pas::isa::Pep10ISA>();
    visit.symTab = QSharedPointer<symbol::Table>::create();
    QSharedPointer<Node> node;
    QVERIFY_THROWS_NO_EXCEPTION(
        [&]() { node = result[0].apply_visitor(visit); }());
    QCOMPARE_NE(node.data(), nullptr);
    bool ret = node->apply(*fn);
    QCOMPARE(ret, true);
  };
  void testVisitor_data() {
    QTest::addColumn<QString>("input");
    QTest::addColumn<QSharedPointer<pas::ops::ConstOp<bool>>>("fn");

    // Blank lines
    QTest::newRow("Blank: no spaces") << "" << isBlank;
    QTest::newRow("Blank: spaces") << " \t" << isBlank;

    // Comment lines
    QTest::newRow("Comment: no spaces") << ";magic" << isComment;
    QTest::newRow("Comment: spaces") << " \t;magic" << isComment;

    /*
     * Unary instructions
     */
    // U type
    QTest::newRow("Unary-U: no spaces") << "ret" << isUType;
    QTest::newRow("Unary-U: spaces") << " \tret" << isUType;
    QTest::newRow("Unary-U: symbol, no spaces") << "s:ret" << isUType;
    QTest::newRow("Unary-U: symbol, spaces") << "s:\t ret" << isUType;
    QTest::newRow("Unary-U: comment, no spaces") << "ret;hi" << isUType;
    QTest::newRow("Unary-U: comment, spaces") << "ret \t;hi" << isUType;

    // R type
    QTest::newRow("Unary-R: no spaces") << "asla" << isRType;
    QTest::newRow("Unary-R: spaces") << " \taslx" << isRType;
    QTest::newRow("Unary-R: symbol, no spaces") << "s:rora" << isRType;
    QTest::newRow("Unary-R: symbol, spaces") << "s:\t rorx" << isRType;
    QTest::newRow("Unary-R: comment, no spaces") << "rola;rola" << isRType;
    QTest::newRow("Unary-R: comment, spaces") << "rolx \t;rolx" << isRType;

    /*
     * NonUnary instructions
     */
    // A type
    QTest::newRow("NonUnary-A: no addr") << "br x" << isAType;
    QTest::newRow("NonUnary-A: addr") << "br x,x" << isAType;
    QTest::newRow("NonUnary-A: symbol, no spaces") << "s:br x,x" << isAType;
    QTest::newRow("NonUnary-A: symbol, spaces") << "s: \tbr x,x" << isAType;
    QTest::newRow("NonUnary-A: comment, no spaces") << "br x,x;x" << isAType;
    QTest::newRow("NonUnary-A: comment, spaces") << "br x,x\t;x" << isAType;

    // AAA type
    QTest::newRow("NonUnary-AAA: addr") << "ldwt x,i" << isAAAType;
    QTest::newRow("NonUnary-AAA: symbol, no spaces")
        << "s:scall x,x" << isAAAType;
    QTest::newRow("NonUnary-AAA: symbol, spaces")
        << "s: \tscall n,x" << isAAAType;
    QTest::newRow("NonUnary-AAA: comment, no spaces")
        << "scall s,x;x" << isAAAType;
    QTest::newRow("NonUnary-AAA: comment, spaces")
        << "scall x,sf\t;x" << isAAAType;

    // RAAA type
    QTest::newRow("NonUnary-RAAA: addr") << "adda x,i" << isRAAAType;
    QTest::newRow("NonUnary-RAAA: symbol, no spaces")
        << "s:addx x,x" << isRAAAType;
    QTest::newRow("NonUnary-RAAA: symbol, spaces")
        << "s: \tsuba n,x" << isRAAAType;
    QTest::newRow("NonUnary-RAAA: comment, no spaces")
        << "subx s,x;x" << isRAAAType;
    QTest::newRow("NonUnary-RAAA: comment, spaces")
        << "ora x,sf\t;x" << isRAAAType;

    /*
     * Directives
     */
    // ALIGN
    QTest::newRow(".ALIGN: mixed case") << ".AlIgN 8" << isAlign;
    QTest::newRow(".ALIGN: symbol") << "s:.align 8" << isAlign;
    QTest::newRow(".ALIGN: comment") << ".ALIGN 8;s" << isAlign;
    // ASCII
    QTest::newRow(".ASCII: mixed case") << ".AsCiI \"h\"" << isASCII;
    QTest::newRow(".ASCII: symbol") << "s:.ASCII \"s\"" << isASCII;
    QTest::newRow(".ASCII: comment") << ".ASCII \"s\";s" << isASCII;
    // QTest::newRow(".ASCII: character") << ".ascii 'a'" << isASCII;
    QTest::newRow(".ASCII: short string") << ".ASCII \"hi\"" << isASCII;
    QTest::newRow(".ASCII: long string") << ".ASCII \"hello\"" << isASCII;
    // BLOCK
    QTest::newRow(".BLOCK: mixed case") << ".BlOcK 10" << isBlock;
    QTest::newRow(".BLOCK: symbol") << "s:.BLOCK 10" << isBlock;
    QTest::newRow(".BLOCK: comment") << ".BLOCK 10;10" << isBlock;
    QTest::newRow(".BLOCK: hex") << ".BLOCK 0x10" << isBlock;
    QTest::newRow(".BLOCK: symbolic") << ".BLOCK hi" << isBlock;
    // TODO: No signed.
    // BURN
    QTest::newRow(".BYTE: mixed case") << ".BuRn 0x10" << isBurn;
    QTest::newRow(".BYTE: comment") << ".BURN 0x10;10" << isBurn;
    // BYTE
    QTest::newRow(".BYTE: mixed case") << ".ByTe 10" << isByte;
    QTest::newRow(".BYTE: symbol") << "s:.BYTE 10" << isByte;
    QTest::newRow(".BYTE: comment") << ".BYTE 10;10" << isByte;
    QTest::newRow(".BYTE: hex") << ".BYTE 0x10" << isByte;
    QTest::newRow(".BYTE: symbolic") << ".BYTE hi" << isByte;
    // END
    QTest::newRow(".END: mixed case") << ".EnD" << isEnd;
    QTest::newRow(".END: comment") << ".END ;hi" << isEnd;
    // EQUATE
    QTest::newRow(".EQUATE: mixed case") << "s:.EQUATE 10" << isEquate;
    QTest::newRow(".EQUATE: comment") << "s:.EQUATE 10;10" << isEquate;
    QTest::newRow(".EQUATE: hex") << "s:.EQUATE 0x10" << isEquate;
    QTest::newRow(".EQUATE: symbolic") << "s:.EQUATE hi" << isEquate;
    // EXPORT
    QTest::newRow(".EXPORT: mixed case") << ".ExPoRt hi" << isExport;
    QTest::newRow(".EXPORT: comment") << ".EXPORT hi ;hi" << isExport;
    // IMPORT
    QTest::newRow(".IMPORT: mixed case") << ".ImPoRt hi" << isImport;
    QTest::newRow(".IMPORT: comment") << ".IMPORT hi;hi" << isImport;
    // INPUT
    QTest::newRow(".INPUT: mixed case") << ".InPuT hi" << isInput;
    QTest::newRow(".INPUT: comment") << ".INPUT hi;hi" << isInput;
    // OUTPUT
    QTest::newRow(".OUTPUT: mixed case") << ".OuTpUt hi" << isOutput;
    QTest::newRow(".OUTPUT: comment") << ".OUTPUT hi;hi" << isOutput;
    // SCALL
    QTest::newRow(".SCALL: mixed case") << ".sCaLl hi" << isSCall;
    QTest::newRow(".SCALL: comment") << ".SCALL hi;10" << isSCall;
    // SECTION
    // TODO: Implement
    // USCALL
    QTest::newRow(".USCALL: mixed case") << ".UsCaLl hi" << isUSCall;
    QTest::newRow(".USCALL: comment") << ".USCALL hi;10" << isUSCall;
    // WORD
    QTest::newRow(".WORD: mixed case") << ".WoRd 10" << isWord;
    QTest::newRow(".WORD: symbol") << "s:.WORD 10" << isWord;
    QTest::newRow(".WORD: comment") << ".WORD 10;10" << isWord;
    QTest::newRow(".WORD: hex") << ".WORD 0x10" << isWord;
    QTest::newRow(".WORD: symbolic") << ".WORD hi" << isWord;
  }
};

#include "node_from_parse_tree.test.moc"

QTEST_MAIN(PasOpsPepp_NodeFromParseTree);
