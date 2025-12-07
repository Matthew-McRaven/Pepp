
/*
 * Copyright (c) 2023-2024 J. Stanley Warford, Matthew McRaven
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

#include "toolchain2/asmb/pep_format.hpp"
#include <catch.hpp>
#include "toolchain2/asmb/pep_lexer.hpp"
#include "toolchain2/asmb/pep_tokens.hpp"
#include "toolchain2/support/lex/buffer.hpp"

using namespace Qt::StringLiterals;
namespace {
static auto idpool = []() { return std::make_shared<pepp::tc::support::StringPool>(); };
static auto data = [](auto str) { return pepp::tc::support::SeekableData{str}; };
} // namespace

TEST_CASE("Pepp ASM source formatting", "[scope:asm][kind:unit][arch:*][tc2]") {
  using Lexer = pepp::tc::lex::PepLexer;
  using Buffer = pepp::tc::lex::Buffer;
  using Checkpoint = pepp::tc::lex::Checkpoint;
  using namespace pepp::tc::lex;
  using pepp::tc::format;
  SECTION("Monadic Instruction") {
    {
      auto l = Lexer(idpool(), data("NOTA ;hi"));
      auto b = Buffer(&l);
      Checkpoint{b};
      CHECK(b.match<Identifier>());
      CHECK(b.match<InlineComment>());
      CHECK(b.match<Empty>());
      auto sp = b.matched_tokens();
      CHECK(format(sp).toStdString() == "         NOTA                ;hi");
    }
    {
      auto l = Lexer(idpool(), data("this: NOTA ;hi"));
      auto b = Buffer(&l);
      Checkpoint{b};
      CHECK(b.match<SymbolDeclaration>());
      CHECK(b.match<Identifier>());
      CHECK(b.match<InlineComment>());
      CHECK(b.match<Empty>());
      auto sp = b.matched_tokens();
      CHECK(format(sp).toStdString() == "this:    NOTA                ;hi");
    }
  }
  SECTION("Dyadic Instruction w/addressing modes") {
    {
      auto l = Lexer(idpool(), data("ADDA 15,d ;hi"));
      auto b = Buffer(&l);
      Checkpoint{b};
      CHECK(b.match<Identifier>());
      CHECK(b.match<Integer>());
      CHECK(b.match<Literal>());
      CHECK(b.match<Identifier>());
      CHECK(b.match<InlineComment>());
      CHECK(b.match<Empty>());
      auto sp = b.matched_tokens();
      CHECK(format(sp).toStdString() == "         ADDA    15,d        ;hi");
    }
    {
      auto l = Lexer(idpool(), data("this:ADDA this,sfx"));
      auto b = Buffer(&l);
      Checkpoint{b};
      CHECK(b.match<SymbolDeclaration>());
      CHECK(b.match<Identifier>());
      CHECK(b.match<Identifier>());
      CHECK(b.match<Literal>());
      CHECK(b.match<Identifier>());
      CHECK(b.match<Empty>());
      auto sp = b.matched_tokens();
      CHECK(format(sp).toStdString() == "this:    ADDA    this,sfx");
    }
    // Fix capitalization on addressing modes and mnemonics.
    {
      auto l = Lexer(idpool(), data("this:addA this,sFx"));
      auto b = Buffer(&l);
      Checkpoint{b};
      CHECK(b.match<SymbolDeclaration>());
      CHECK(b.match<Identifier>());
      CHECK(b.match<Identifier>());
      CHECK(b.match<Literal>());
      CHECK(b.match<Identifier>());
      CHECK(b.match<Empty>());
      auto sp = b.matched_tokens();
      CHECK(format(sp).toStdString() == "this:    ADDA    this,sfx");
    }
  }
  SECTION("Dyadic Instruction w/o addressing modes") {
    // Illegal assembly code which demonstrates that formatting does not depend on program being correct.
    auto l = Lexer(idpool(), data("this:ADDA this"));
    auto b = Buffer(&l);
    Checkpoint{b};
    CHECK(b.match<SymbolDeclaration>());
    CHECK(b.match<Identifier>());
    CHECK(b.match<Identifier>());
    CHECK(b.match<Empty>());
    auto sp = b.matched_tokens();
    CHECK(format(sp).toStdString() == "this:    ADDA    this");
  }
  SECTION(".SECTION") {
    // Illegal assembly code which demonstrates that formatting does not depend on program being correct.
    auto l = Lexer(idpool(), data(R"(.SECTION "text",    "rx")"));
    auto b = Buffer(&l);
    Checkpoint{b};
    CHECK(b.match<DotCommand>());
    CHECK(b.match<StringConstant>());
    CHECK(b.match<Literal>());
    CHECK(b.match<StringConstant>());
    CHECK(b.match<Empty>());
    auto sp = b.matched_tokens();
    CHECK(format(sp).toStdString() == R"(         .SECTION "text", "rx")");
  }
  SECTION(".ASCII") {
    auto l = Lexer(idpool(), data(R"(execErr:   .ascii "Main failed with return value \0"  )"));
    auto b = Buffer(&l);
    Checkpoint{b};
    CHECK(b.match<SymbolDeclaration>());
    CHECK(b.match<DotCommand>());
    CHECK(b.match<StringConstant>());
    CHECK(b.match<Empty>());
    auto sp = b.matched_tokens();
    CHECK(format(sp).toStdString() == R"(execErr: .ASCII  "Main failed with return value \0")");
  }
  SECTION("Comment-only") {
    auto l = Lexer(idpool(), data(R"(    ;******* STRO)"));
    auto b = Buffer(&l);
    Checkpoint{b};
    CHECK(b.match<InlineComment>());
    CHECK(b.match<Empty>());
    auto sp = b.matched_tokens();
    CHECK(format(sp).toStdString() == R"(;******* STRO)");
  }
}
