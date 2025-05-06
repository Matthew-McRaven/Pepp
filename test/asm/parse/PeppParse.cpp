/*
 * Copyright (c) 2024 J. Stanley Warford, Matthew McRaven
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

#include <catch.hpp>
#include <iostream>
#include "antlr4-runtime.h"

#include "toolchain/parse/PeppLexer.h"
#include "toolchain/parse/PeppLexerErrorListener.h"
#include "toolchain/parse/PeppParser.h"

using namespace antlr4;
using namespace parse;

namespace {
void single_line(std::string text, std::string output) {
  ANTLRInputStream input(text);
  PeppLexer lexer(&input);
  CommonTokenStream tokens(&lexer);
  PeppLexerErrorListener listener{};
  lexer.addErrorListener(&listener);
  PeppParser parser(&tokens);
  // Test cases includ macro symbols, macro identifiers. Macro expansion must be deferred.
  parser.allow_deferred_macros = true;
  auto *tree = parser.prog();
  REQUIRE(!listener.hadError());
  REQUIRE(output == tree->toStringTree(&parser));
}
} // namespace

TEST_CASE("Pepp blank parsing", "[scope:asm][kind:unit][arch:pep10]") {
  using namespace parse;
  auto [text, type] = GENERATE(table<std::string, std::string>({
      {"\n", "(prog \\n <EOF>)"},
      {"\r\n", "(prog \\r\\n <EOF>)"},
      {"\n\n", "(prog \\n \\n <EOF>)"},
  }));
  single_line(text, type);
}

TEST_CASE("Pepp comment parsing", "[scope:asm][kind:unit][arch:pep10]") {
  using namespace parse;
  auto [text, type] = GENERATE(table<std::string, std::string>({
      {";oneword", "(prog (stat ;oneword) <EOF>)"},
      {";two words", "(prog (stat ;two words) <EOF>)"},
      {"; ignore ;comment in comment", "(prog (stat ; ignore ;comment in comment) <EOF>)"},
  }));
  single_line(text, type);
}
TEST_CASE("Pepp unary parsing", "[scope:asm][kind:unit][arch:pep10]") {
  using namespace parse;
  auto [text, type] = GENERATE(table<std::string, std::string>({
      // Do not test for validity of instruction, a seaparate semantic analysis pass handles this.
      {"ASRA", "(prog (stat (instruction ASRA)) <EOF>)"},
      {"ROLA ;comment", "(prog (stat (instruction ROLA) ;comment) <EOF>)"},
      {"NOTI:ASRA", "(prog (stat (symbol NOTI:) (instruction ASRA)) <EOF>)"},
      {"$1:CMD", "(prog (stat (symbol $1:) (instruction CMD)) <EOF>)"},
  }));
  single_line(text, type);
}
TEST_CASE("Pepp non-unary parsing", "[scope:asm][kind:unit][arch:pep10]") {
  using namespace parse;
  auto [text, type] = GENERATE(table<std::string, std::string>({
      // non-unary, no addressing mode
      {"BR 10", "(prog (stat (instruction BR (argument 10))) <EOF>)"},
      {"BRC 0x20", "(prog (stat (instruction BRC (argument 0x20))) <EOF>)"},
      {"BRt 'h'", "(prog (stat (instruction BRt (argument 'h'))) <EOF>)"},
      {"instr \"hi\"", "(prog (stat (instruction instr (argument \"hi\"))) <EOF>)"},
      {"BR $1", "(prog (stat (instruction BR (argument $1))) <EOF>)"},
      {"BR ident", "(prog (stat (instruction BR (argument ident))) <EOF>)"},
      {"$1:BR 10", "(prog (stat (symbol $1:) (instruction BR (argument 10))) <EOF>)"},
      {"sym:BR 10", "(prog (stat (symbol sym:) (instruction BR (argument 10))) <EOF>)"},
      {"BR ident\t\t;hi", "(prog (stat (instruction BR (argument ident)) ;hi) <EOF>)"},
      // non-unary, with addressing mode
      {"BR ident,I", "(prog (stat (instruction BR (argument ident) , I)) <EOF>)"},
      {"$1:BR 10,d", "(prog (stat (symbol $1:) (instruction BR (argument 10) , d)) <EOF>)"},
      {"sym:BR 10,test", "(prog (stat (symbol sym:) (instruction BR (argument 10) , test)) <EOF>)"},
  }));
  single_line(text, type);
}
TEST_CASE("Pepp directive parsing", "[scope:asm][kind:unit][arch:pep10]") {
  using namespace parse;
  auto [text, type] = GENERATE(table<std::string, std::string>({
      {".END", "(prog (stat (directive .END)) <EOF>)"},
      {".WORD 10", "(prog (stat (directive .WORD (argument_list (argument 10)))) <EOF>)"},
      {".SECTIOn asdx, \"RWX\"",
       "(prog (stat (directive .SECTIOn (argument_list (argument asdx) , (argument \"RWX\")))) <EOF>)"},
      {"symbol:.ARGL 1, 'a', ident, $1, 0x20;comment",
       "(prog (stat (symbol symbol:) (directive .ARGL (argument_list (argument 1) , (argument 'a') , (argument ident) "
       ", (argument $1) , (argument 0x20))) ;comment) <EOF>)"},
  }));
  single_line(text, type);
}

TEST_CASE("Pepp macro parsing", "[scope:asm][kind:unit][arch:pep10]") {
  using namespace parse;
  auto [text, type] = GENERATE(table<std::string, std::string>({
      {"@hi", "(prog (stat (invoke_macro @hi)) <EOF>)"},
      {"sym:@hi", "(prog (stat (symbol sym:) (invoke_macro @hi)) <EOF>)"},
      {"$1:@hi $1,2",
       "(prog (stat (symbol $1:) (invoke_macro @hi (argument_list (argument $1) , (argument 2)))) <EOF>)"},
      {"@hi 10,d;comment",
       "(prog (stat (invoke_macro @hi (argument_list (argument 10) , (argument d))) ;comment) <EOF>)"},
  }));
  single_line(text, type);
}
