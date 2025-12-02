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

#include "./parse.hpp"
#include <tuple>

#include <QRegularExpression>

//
static const QRegularExpression macrodecl =
    QRegularExpression(R"(^[ \t]*@([a-zA-Z][a-zA-Z0-9_]*)[ \t]+([0-9]|1[0-6])[ \t]*$)");

std::tuple<bool, QString, quint8> macro::analyze_macro_definition(QString macro_text) {
  /*
   * A macro file must begin with with name of the macro, followed by an
   * arbitrary number of spaces followed by an integer in [0,16] specifying the
   * number of arguments the macro takes.
   *
   * Neither comments nor whitespace may occur before this definition line.
   *
   * Below are valid examples:
   * @DECI 2
   * @UNOP 0
   *
   * Below are invalid examples, with comments descrbing why.
   *
   * Whitepace preceding macro definiton:
   *      @DECI 2
   * Space between macro name and macro symbol @.
   * @ deci 2
   *
   * Line ends in a comment
   * @deci 2 ;My comment
   *
   */

  QStringView first_line;
  if (macro_text.indexOf("\n") == -1) first_line = macro_text;
  else first_line = QStringView(macro_text).left(macro_text.indexOf("\n"));
  auto match = macrodecl.matchView(first_line);

  if (!match.hasMatch()) return {false, QString(), 0};
  auto name = match.captured(1).toUpper();
  bool success = false;
  auto arg_count = match.captured(2).toInt(&success);
  return {success, name.toUpper(), arg_count};
}
