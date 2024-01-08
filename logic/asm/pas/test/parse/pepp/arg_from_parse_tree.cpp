/*
 * Copyright (c) 2023 J. Stanley Warford, Matthew McRaven
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <QObject>
#include <QTest>

#include "asm/pas/ast/value/character.hpp"
#include "asm/pas/ast/value/decimal.hpp"
#include "asm/pas/ast/value/hexadecimal.hpp"
#include "asm/pas/ast/value/identifier.hpp"
#include "asm/pas/ast/value/string.hpp"
#include "asm/pas/ast/value/symbolic.hpp"
#include "asm/pas/parse/pepp/arg_from_parse_tree.hpp"
#include "asm/pas/parse/pepp/rules_values.hpp"
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
    visit.symTab = QSharedPointer<symbol::Table>::create(2);
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

#include "arg_from_parse_tree.moc"

QTEST_MAIN(PasParsePepp_ArgFromParseTree);
