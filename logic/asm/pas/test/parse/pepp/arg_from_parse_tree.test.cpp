#include <QObject>
#include <QTest>

#include "pas/ast/value/character.hpp"
#include "pas/ast/value/decimal.hpp"
#include "pas/ast/value/hexadecimal.hpp"
#include "pas/ast/value/identifier.hpp"
#include "pas/ast/value/string.hpp"
#include "pas/ast/value/symbolic.hpp"
#include "pas/parse/pepp/arg_from_parse_tree.hpp"
#include "pas/parse/pepp/rules_values.hpp"
using namespace pas::parse::pepp;
using namespace pas::ast::value;
class PasParsePepp_ArgFromParseTree : public QObject {
  Q_OBJECT
private slots:
  void character() {
    QFETCH(QString, input);
    QFETCH(bool, willCast);
    auto asStd = input.toStdString();
    Value result;
    QVERIFY(parse(asStd.begin(), asStd.end(), argument, result));
    auto visit = pas::parse::pepp::ParseToArg();
    auto ret = result.apply_visitor(visit);
    if (willCast)
      QCOMPARE_NE(dynamic_cast<Character *>(ret.data()), nullptr);
    else
      QCOMPARE(dynamic_cast<Character *>(ret.data()), nullptr);
  }
  void character_data() {
    QTest::addColumn<QString>("input");
    QTest::addColumn<bool>("willCast");

    QTest::newRow("standard ASCII") << u"'a'"_qs << true;
    QTest::newRow("escaped") << u"'\\t'"_qs << true;
    QTest::newRow("hexcode") << u"'\\X00'"_qs << true;
  }

  void shortstring() {
    QFETCH(QString, input);
    QFETCH(bool, willCast);
    auto asStd = input.toStdString();
    Value result;
    QVERIFY(parse(asStd.begin(), asStd.end(), argument, result));
    auto visit = pas::parse::pepp::ParseToArg();
    auto ret = result.apply_visitor(visit);
    if (willCast)
      QCOMPARE_NE(dynamic_cast<ShortString *>(ret.data()), nullptr);
    else
      QCOMPARE(dynamic_cast<ShortString *>(ret.data()), nullptr);
  }
  void shortstring_data() {
    QTest::addColumn<QString>("input");
    QTest::addColumn<bool>("willCast");

    QTest::newRow("standard ASCII") << u"\"a\""_qs << true;
    QTest::newRow("escaped") << u"\"\\t\""_qs << true;
    QTest::newRow("hexcode") << u"\"\\X00\""_qs << true;
    QTest::newRow("two chars ASCII") << u"\"aa\""_qs << true;
    QTest::newRow("three chars ASCII") << u"\"aaa\""_qs << false;
  }

  void longstring() {
    QFETCH(QString, input);
    QFETCH(bool, willCast);
    auto asStd = input.toStdString();
    Value result;
    QVERIFY(parse(asStd.begin(), asStd.end(), argument, result));
    auto visit = pas::parse::pepp::ParseToArg();
    auto ret = result.apply_visitor(visit);
    if (willCast)
      QCOMPARE_NE(dynamic_cast<LongString *>(ret.data()), nullptr);
    else
      QCOMPARE(dynamic_cast<LongString *>(ret.data()), nullptr);
  }
  void longstring_data() {
    QTest::addColumn<QString>("input");
    QTest::addColumn<bool>("willCast");

    QTest::newRow("standard ASCII") << u"\"aaa\""_qs << true;
    QTest::newRow("escaped") << u"\"aa\\t\""_qs << true;
    QTest::newRow("hexcode") << u"\"aa\\X00\""_qs << true;
  }

  void unsigned_decimal() {
    QFETCH(QString, input);
    QFETCH(bool, willCast);
    auto asStd = input.toStdString();
    Value result;
    QVERIFY(parse(asStd.begin(), asStd.end(), argument, result));
    auto visit = pas::parse::pepp::ParseToArg();
    auto ret = result.apply_visitor(visit);
    if (willCast)
      QCOMPARE_NE(dynamic_cast<UnsignedDecimal *>(ret.data()), nullptr);
    else
      QCOMPARE(dynamic_cast<UnsignedDecimal *>(ret.data()), nullptr);
  }
  void unsigned_decimal_data() {
    QTest::addColumn<QString>("input");
    QTest::addColumn<bool>("willCast");

    QTest::newRow("0") << u"0"_qs << true;
    QTest::newRow("max") << u"65536"_qs << true;
  }

  void signed_decimal() {
    QFETCH(QString, input);
    QFETCH(bool, willCast);
    auto asStd = input.toStdString();
    Value result;
    QVERIFY(parse(asStd.begin(), asStd.end(), argument, result));
    auto visit = pas::parse::pepp::ParseToArg();
    auto ret = result.apply_visitor(visit);
    if (willCast)
      QCOMPARE_NE(dynamic_cast<SignedDecimal *>(ret.data()), nullptr);
    else
      QCOMPARE(dynamic_cast<SignedDecimal *>(ret.data()), nullptr);
  }
  void signed_decimal_data() {
    QTest::addColumn<QString>("input");
    QTest::addColumn<bool>("willCast");

    QTest::newRow("min") << u"-32768"_qs << true;
    QTest::newRow("-0") << u"-0"_qs << true;
  }

  void hexadecimal() {
    QFETCH(QString, input);
    QFETCH(bool, willCast);
    auto asStd = input.toStdString();
    Value result;
    QVERIFY(parse(asStd.begin(), asStd.end(), argument, result));
    auto visit = pas::parse::pepp::ParseToArg();
    auto ret = result.apply_visitor(visit);
    if (willCast)
      QCOMPARE_NE(dynamic_cast<Hexadecimal *>(ret.data()), nullptr);
    else
      QCOMPARE(dynamic_cast<Hexadecimal *>(ret.data()), nullptr);
  }
  void hexadecimal_data() {
    QTest::addColumn<QString>("input");
    QTest::addColumn<bool>("willCast");

    QTest::newRow("0") << u"0x0"_qs << true;
    QTest::newRow("max") << u"0xfFfF"_qs << true;
  }

  void identifier() {
    QFETCH(QString, input);
    QFETCH(bool, willCast);
    auto asStd = input.toStdString();
    Value result;
    QVERIFY(parse(asStd.begin(), asStd.end(), argument, result));
    auto visit = pas::parse::pepp::ParseToArg();
    visit.preferIdent = true;
    auto ret = result.apply_visitor(visit);
    if (willCast)
      QCOMPARE_NE(dynamic_cast<pas::ast::value::Identifier *>(ret.data()),
                  nullptr);
    else
      QCOMPARE(dynamic_cast<pas::ast::value::Identifier *>(ret.data()),
               nullptr);
  }
  void identifier_data() {
    QTest::addColumn<QString>("input");
    QTest::addColumn<bool>("willCast");

    QTest::newRow("hello") << u"hello"_qs << true;
  }

  void symbolic() {
    QFETCH(QString, input);
    QFETCH(bool, willCast);
    auto asStd = input.toStdString();
    Value result;
    QVERIFY(parse(asStd.begin(), asStd.end(), argument, result));
    auto visit = pas::parse::pepp::ParseToArg();
    visit.symTab = QSharedPointer<symbol::Table>::create();
    auto ret = result.apply_visitor(visit);
    if (willCast)
      QCOMPARE_NE(dynamic_cast<Symbolic *>(ret.data()), nullptr);
    else
      QCOMPARE(dynamic_cast<Symbolic *>(ret.data()), nullptr);
  }
  void symbolic_data() {
    QTest::addColumn<QString>("input");
    QTest::addColumn<bool>("willCast");

    QTest::newRow("hello") << u"hello"_qs << true;
  }
};

#include "arg_from_parse_tree.test.moc"

QTEST_MAIN(PasParsePepp_ArgFromParseTree);
