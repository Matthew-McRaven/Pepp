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

#include "pas/parse/pepp/rules_lines.hpp"

using namespace pas::parse::pepp;
using namespace std::literals::string_literals;
class PasParserPeppLines : public QObject {
  Q_OBJECT
private slots:
  void testBlank() {
    auto value = "  \t"s;
    BlankType result;
    QVERIFY(parse(value.begin(), value.end(), blank, result));
  }
  void testComment() {
    auto value = ";magic"s;
    CommentType result;
    QVERIFY(parse(value.begin(), value.end(), comment, result));
  }

  void testUnary() {
    auto values = QList<std::string>{"hi"s, "symbol:hi", "symbol:hi;comment",
                                     "a;comment"};
    for (const auto &value : values) {
      UnaryType result;
      QVERIFY2(parse(value.begin(), value.end(), unary, result), value.data());
      qWarning() << QString::fromStdString(result.comment) << result.hasComment
                 << QString::fromStdString(result.identifier)
                 << QString::fromStdString(result.symbol);
    }
  }

  void testNonUnary() {
    auto values =
        QList<std::string>{"hi 10"s, "symbol:hi 10", "symbol:hi 10;comme nt",
                           "a 10;comment", "hi 10,s"};
    for (const auto &value : values) {
      NonUnaryType result;
      QVERIFY2(parse(value.begin(), value.end(), nonunary, result),
               value.data());
      qWarning() << QString::fromStdString(result.comment)
                 << QString::fromStdString(result.identifier)
                 << result.hasComment
                 << (boost::get<DecimalLiteral>(result.arg).value)
                 << QString::fromStdString(result.addr)
                 << QString::fromStdString(result.symbol);
    }
  }
  void testDirective() {
    auto values =
        QList<std::string>{"sym:.hi", "sym: .hi", ".hi 10", ".hi 10, 20"};
    for (const auto &value : values) {
      DirectiveType result;
      QVERIFY2(parse(value.begin(), value.end(), directive, result),
               value.data());
      qWarning() << QString::fromStdString(result.comment)
                 << QString::fromStdString(result.identifier)
                 << result.hasComment << result.args.size()
                 << QString::fromStdString(result.symbol);
    }
  }
  void testMacro() {
    auto values =
        QList<std::string>{"sym:@hi", "sym: @hi", "@hi 10", "@hi 10, 20"};
    for (const auto &value : values) {
      MacroType result;
      QVERIFY2(parse(value.begin(), value.end(), macro, result), value.data());
      qWarning() << QString::fromStdString(result.comment)
                 << QString::fromStdString(result.identifier)
                 << result.hasComment << result.args.size()
                 << QString::fromStdString(result.symbol);
    }
  }
};

#include "parser_lines.moc"

QTEST_MAIN(PasParserPeppLines);
