
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

#include "macro/parse.hpp"

#include <QTest>
#include <tuple>
class MacroParser : public QObject {
  Q_OBJECT
private slots:
  void acceptsValidSpaces() {
    QCOMPARE(std::get<0>(macro::analyze_macro_definition(u"@deci 0"_qs)), true);
    QCOMPARE(std::get<0>(macro::analyze_macro_definition(u"@deci 	0"_qs)),
             true);
    QCOMPARE(std::get<0>(macro::analyze_macro_definition(u"@deci 0	"_qs)),
             true);
    QCOMPARE(std::get<0>(macro::analyze_macro_definition(u"@deci  0"_qs)),
             true);
    QCOMPARE(std::get<0>(macro::analyze_macro_definition(u"@deci  0  "_qs)),
             true);
    QCOMPARE(std::get<0>(
                 macro::analyze_macro_definition(u"@deci	0	"_qs)),
             true);
  }
  void rejectsInvalidSpaces() {

    QCOMPARE(std::get<0>(macro::analyze_macro_definition(u"@deco​0​"_qs)),
             false); // 0-width space before and after
    QCOMPARE(std::get<0>(macro::analyze_macro_definition(u" @deci 0"_qs)),
             false); // Can't have whitespace before
    QCOMPARE(std::get<0>(macro::analyze_macro_definition(u"@ deci 0"_qs)),
             false); // Can't have whitespace between @ and name
  }
  void handlesDifferentArity() {
    auto _0ar = macro::analyze_macro_definition(u"@deci 0"_qs);
    auto _8ar = macro::analyze_macro_definition(u"@deco 8"_qs);
    QVERIFY(std::get<0>(_0ar));
    QCOMPARE(std::get<1>(_0ar).toUtf8().toStdString(), "deci");
    QCOMPARE(std::get<2>(_0ar), 0);
    QVERIFY(std::get<0>(_8ar));
    QCOMPARE(std::get<1>(_8ar).toUtf8().toStdString(), "deco");
    QCOMPARE(std::get<2>(_8ar), 8);
  }
  void rejectsComments() {
    QCOMPARE(std::get<0>(macro::analyze_macro_definition("@deci 2 ;fail")),
             false);
  }
  // TODO: Can parse a macro with a body
  // TODO: Requires arg count
  // TODO: Requires @
  // TODO: Rejects symbols, other chars.
};
#include "parse.moc"

QTEST_MAIN(MacroParser)
