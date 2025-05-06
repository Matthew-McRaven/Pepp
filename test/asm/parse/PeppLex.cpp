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

using namespace antlr4;
using namespace parse;

namespace {
void single_token(std::string text, uint64_t type) {
  ANTLRInputStream input(text);
  PeppLexer lexer(&input);
  CommonTokenStream stream(&lexer);
  PeppLexerErrorListener listener{};
  lexer.addErrorListener(&listener);
  stream.fill();
  REQUIRE(!listener.hadError());
  auto tokens = stream.getTokens();
  REQUIRE(tokens.size() == 2);
  REQUIRE(tokens[0]->getType() == type);
  REQUIRE(tokens[0]->getText() == text);
}
} // namespace

TEST_CASE("Pepp numeric lexing", "[scope:asm][kind:unit][arch:pep10]") {
  using namespace parse;
  auto [text, type] = GENERATE(table<std::string, int>({
      // We shouldn't reject large integer values (>16b) during lexing.
      {"0xFEED", PeppLexer::HEXADECIMAL},
      {"0x00FEED", PeppLexer::HEXADECIMAL},
      {"0xF00FEED", PeppLexer::HEXADECIMAL},
      {"0", PeppLexer::UNSIGNED_DECIMAL},
      {"20", PeppLexer::UNSIGNED_DECIMAL},
      {"100000", PeppLexer::UNSIGNED_DECIMAL},
      {"-20", PeppLexer::SIGNED_DECIMAL},
      {"-0", PeppLexer::SIGNED_DECIMAL},
      {"-100000", PeppLexer::SIGNED_DECIMAL},
  }));
  single_token(text, type);
}

TEST_CASE("Pepp identifier lexing", "[scope:asm][kind:unit][arch:pep10]") {

  using namespace parse;
  auto [text, type] = GENERATE(table<std::string, int>({
      {"identifier", PeppLexer::IDENTIFIER},
      {"@macro", PeppLexer::AT_IDENTIFIER},
      {".directive", PeppLexer::DOT_IDENTIFIER},
      {"symbol:", PeppLexer::SYMBOL},
      {";", PeppLexer::COMMENT},
      {";singleword", PeppLexer::COMMENT},
      {"; multiple words", PeppLexer::COMMENT},
      {"$0:", PeppLexer::PLACEHOLDER_SYMBOL},
      {"$1:", PeppLexer::PLACEHOLDER_SYMBOL},
      {"$9:", PeppLexer::PLACEHOLDER_SYMBOL},
  }));
  single_token(text, type);
}

TEST_CASE("Pepp argument lexing", "[scope:asm][kind:unit][arch:pep10]") {

  using namespace parse;
  auto [text, type] = GENERATE(table<std::string, int>({
      {"$0", PeppLexer::PLACEHOLDER_MACRO},
      {"$1", PeppLexer::PLACEHOLDER_MACRO},
      {"$9", PeppLexer::PLACEHOLDER_MACRO},
      {",", PeppLexer::COMMA},
  }));
  single_token(text, type);
}

TEST_CASE("Pepp character sequence lexing", "[scope:asm][kind:unit][arch:pep10]") {

  using namespace parse;
  auto [text, type] = GENERATE(table<std::string, int>({
      {R"('a')", PeppLexer::CHARACTER},    {R"(' ')", PeppLexer::CHARACTER},  {R"('\t')", PeppLexer::CHARACTER},
      {R"('\r')", PeppLexer::CHARACTER},   {R"('\n')", PeppLexer::CHARACTER}, {R"('\'')", PeppLexer::CHARACTER},
      {R"('\0')", PeppLexer::CHARACTER},   {R"('\"')", PeppLexer::CHARACTER}, {R"('\x00')", PeppLexer::CHARACTER},
      {R"('\XfF')", PeppLexer::CHARACTER}, {R"("")", PeppLexer::STRING},      {R"("a")", PeppLexer::STRING},
      {R"(" ")", PeppLexer::STRING},       {R"("\t")", PeppLexer::STRING},    {R"("\0")", PeppLexer::STRING},
      {R"("\r")", PeppLexer::STRING},      {R"("\n")", PeppLexer::STRING},    {R"("\'")", PeppLexer::STRING},
      {R"("\"")", PeppLexer::STRING},      {R"("\x00")", PeppLexer::STRING},  {R"("\XfF")", PeppLexer::STRING},
      {R"("aa")", PeppLexer::STRING},
  }));
  single_token(text, type);
}

TEST_CASE("Pepp character sequence lexing edge cases", "[scope:asm][kind:unit][arch:pep10]") {
  using namespace parse;
  auto [text, type] = GENERATE(table<std::string, int>({
      {R"('')", PeppLexer::CHARACTER},
      {R"('\x0')", PeppLexer::CHARACTER},
      {R"('aa')", PeppLexer::CHARACTER},

  }));
  ANTLRInputStream input(text);
  PeppLexer lexer(&input);
  // Remove default lexers (which write to STDERR).
  lexer.removeErrorListeners();
  CommonTokenStream stream(&lexer);
  PeppLexerErrorListener listener{};
  lexer.addErrorListener(&listener);
  stream.fill();
  REQUIRE(listener.hadError());
}
