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

// This is a hack to get around QT using keywords such as emit.
#undef emit

#include "antlr4-runtime.h"

#include "detail/MacroLexer.h"
#include "detail/MacroLexerErrorListener.h"
#include "detail/MacroParser.h"

using namespace antlr4;
using namespace macro;
using namespace macro::detail;

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
  // Append a newline to ensure that our find operation always succeds.
  auto as_std = macro_text.toUtf8().toStdString() + "\n";
  // Include the newline in the text we pass to the lexer to make our grammar simpler.
  std::string text = as_std.substr(0, as_std.find("\n") + 1);
  ANTLRInputStream input(text);
  MacroLexer lexer(&input);
  // Remove listener that writes to stderr.
  lexer.removeErrorListeners();
  MacroLexerErrorListener listener{};
  lexer.addErrorListener(&listener);

  CommonTokenStream tokens(&lexer);
  MacroParser parser(&tokens);
  auto *tree = parser.decl();
  if (listener.hadError()) return {false, QString(), 0};
  // Strip ampersand to be left with macro name as identifier.
  auto name = QString::fromStdString(tree->AT_IDENTIFIER()->getText()).replace("@", "");
  bool arg_convert = true;
  auto arg_count = QString ::fromStdString(tree->UNSIGNED_DECIMAL()->getText()).toInt(&arg_convert, 10);

  return {arg_convert, name, arg_count};
}
