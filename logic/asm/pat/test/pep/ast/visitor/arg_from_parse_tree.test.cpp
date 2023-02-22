#include "pat/pep/ast/visitor/arg_from_parse_tree.hpp"
#include "pat/pep/parse/rules_values.hpp"
#include <QTest>
#include <QtCore>
using namespace pat::pep::ast;
class PepAstVisitor_ArgFromParseTree : public QObject {
  Q_OBJECT
  pat::pep::parse::Value toValue(QString string) {
    auto stdStr = string.toUtf8().toStdString();
    pat::pep::parse::Value ret;
    Q_ASSERT(
        parse(stdStr.begin(), stdStr.end(), pat::pep::parse::argument, ret));
    return ret;
  }
private slots:
  void testChar() {
    auto value = toValue(u"'a'"_qs);
    auto visit = visitor::ParseToArg();
    auto arg = value.apply_visitor(visit);
    auto asTyped = qSharedPointerCast<pat::ast::argument::Character>(arg);
    QCOMPARE_NE(asTyped, nullptr);
  }

  void testShortString() {
    auto value = toValue(u"\"ab\""_qs);
    auto visit = visitor::ParseToArg();
    auto arg = value.apply_visitor(visit);
    auto asTyped = qSharedPointerCast<pat::ast::argument::ShortString>(arg);
    QCOMPARE_NE(asTyped, nullptr);
  }

  void testLongString() {
    auto value = toValue(u"\"abc\""_qs);
    auto visit = visitor::ParseToArg();
    auto arg = value.apply_visitor(visit);
    auto asTyped = qSharedPointerCast<pat::ast::argument::LongString>(arg);
    QCOMPARE_NE(asTyped, nullptr);
  }

  void testSignedDecimal() {
    auto value = toValue(u"-10"_qs);
    auto visit = visitor::ParseToArg();
    auto arg = value.apply_visitor(visit);
    auto asTyped = qSharedPointerCast<pat::ast::argument::SignedDecimal>(arg);
    QCOMPARE_NE(asTyped, nullptr);
  }

  void testUnsignedDecimal() {
    auto value = toValue(u"10"_qs);
    auto visit = visitor::ParseToArg();
    auto arg = value.apply_visitor(visit);
    auto asTyped = qSharedPointerCast<pat::ast::argument::UnsignedDecimal>(arg);
    QCOMPARE_NE(asTyped, nullptr);
  }

  void testHexadecimal() {
    auto value = toValue(u"0x10"_qs);
    auto visit = visitor::ParseToArg();
    auto arg = value.apply_visitor(visit);
    auto asTyped = qSharedPointerCast<pat::ast::argument::Hexadecimal>(arg);
    QCOMPARE_NE(asTyped, nullptr);
  }

  void testSymbol() {
    auto value = toValue(u"ab"_qs);
    auto visit = visitor::ParseToArg();
    visit.preferIdent = false;
    visit.symTab = QSharedPointer<symbol::Table>::create();
    auto arg = value.apply_visitor(visit);
    auto asTyped = qSharedPointerCast<pat::ast::argument::Symbolic>(arg);
    QCOMPARE_NE(asTyped, nullptr);
  }

  void testIdentifier() {
    auto value = toValue(u"cd"_qs);
    auto visit = visitor::ParseToArg();
    visit.preferIdent = true;
    auto arg = value.apply_visitor(visit);
    auto asTyped = qSharedPointerCast<pat::ast::argument::Identifier>(arg);
    QCOMPARE_NE(asTyped, nullptr);
  }
};
#include "arg_from_parse_tree.test.moc"

QTEST_MAIN(PepAstVisitor_ArgFromParseTree);
