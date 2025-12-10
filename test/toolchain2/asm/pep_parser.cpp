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

#include "toolchain2/asmb/pep_parser.hpp"
#include <catch.hpp>
#include "toolchain/pas/ast/value/numeric.hpp"

using namespace Qt::StringLiterals;
namespace {
static auto data = [](auto str) { return pepp::tc::support::SeekableData{str}; };
} // namespace

TEST_CASE("Pepp ASM parser", "[scope:asm][kind:unit][arch:*][tc2]") {
  using Lexer = pepp::tc::lex::PepLexer;
  using Parser = pepp::tc::parser::PepParser;
  using SymbolTable = symbol::Table;
  using namespace pepp::tc::ir;
  SECTION("No input") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(" "));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    CHECK(std::dynamic_pointer_cast<EmptyLine>(results[0]));
  }
  SECTION("Empty lines") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("   \n   "));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 2);
    CHECK(std::dynamic_pointer_cast<EmptyLine>(results[0]));
    CHECK(std::dynamic_pointer_cast<EmptyLine>(results[1]));
  }
  SECTION("Monadic instructions") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("NOTA"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    auto r0 = std::dynamic_pointer_cast<MonadicInstruction>(results[0]);
    REQUIRE(r0);
    CHECK(r0->mnemonic.instruction == isa::detail::pep10::Mnemonic::NOTA);
  }
  SECTION("Monadic instructions declaring symbols") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("symb: NOTA"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    auto r0 = std::dynamic_pointer_cast<MonadicInstruction>(results[0]);
    REQUIRE(r0);
    CHECK(r0->mnemonic.instruction == isa::detail::pep10::Mnemonic::NOTA);
    auto attr = r0->attribute(attr::Type::SymbolDeclaration);
    REQUIRE(attr);
    auto sym = (pepp::tc::ir::attr::SymbolDeclaration *)attr;
    CHECK(sym->entry->name.toStdString() == "symb");
    CHECK(sym->entry->state == symbol::DefinitionState::kSingle);
  }
  SECTION("Dyadic instructions") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("ADDA 10,i"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    auto r0 = std::dynamic_pointer_cast<DyadicInstruction>(results[0]);
    REQUIRE(r0);
    CHECK(r0->mnemonic.instruction == isa::detail::pep10::Mnemonic::ADDA);
    CHECK(r0->addr_mode.addr_mode == isa::detail::pep10::AddressingMode::I);
    CHECK(std::dynamic_pointer_cast<pas::ast::value::Numeric>(r0->argument.value));
  }
  SECTION("Dyadic instructions with optional addressing modes") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("BR 10"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    auto r0 = std::dynamic_pointer_cast<DyadicInstruction>(results[0]);
    REQUIRE(r0);
    CHECK(r0->mnemonic.instruction == isa::detail::pep10::Mnemonic::BR);
    CHECK(r0->addr_mode.addr_mode == isa::detail::pep10::AddressingMode::I);
    CHECK(std::dynamic_pointer_cast<pas::ast::value::Numeric>(r0->argument.value));
  }
  SECTION("Dyadic instructions with large argument") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("BR 0xffff"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    auto r0 = std::dynamic_pointer_cast<DyadicInstruction>(results[0]);
    REQUIRE(r0);
    CHECK(r0->mnemonic.instruction == isa::detail::pep10::Mnemonic::BR);
    CHECK(r0->addr_mode.addr_mode == isa::detail::pep10::AddressingMode::I);
    CHECK(std::dynamic_pointer_cast<pas::ast::value::Numeric>(r0->argument.value));
  }
  SECTION("Dyadic instructions with symbolic argument") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("this:BR this,x"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    auto r0 = std::dynamic_pointer_cast<DyadicInstruction>(results[0]);
    REQUIRE(r0);
    CHECK(r0->mnemonic.instruction == isa::detail::pep10::Mnemonic::BR);
    CHECK(r0->addr_mode.addr_mode == isa::detail::pep10::AddressingMode::X);
    CHECK(std::dynamic_pointer_cast<pas::ast::value::Symbolic>(r0->argument.value));
  }
}

TEST_CASE("Pepp ASM parser dot commands", "[scope:asm][kind:unit][arch:*][tc2]") {
  using Lexer = pepp::tc::lex::PepLexer;
  using Parser = pepp::tc::parser::PepParser;
  using SymbolTable = symbol::Table;
  using namespace pepp::tc::ir;

  SECTION(".ALIGN") {
    {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data(".ALIGN 1"));
      auto results = p.parse(diag);
      CHECK(diag.count() == 0);
      REQUIRE(results.size() == 1);
      CHECK(std::dynamic_pointer_cast<DotAlign>(results[0]));
    }
    {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data("s: .ALIGN 4"));
      auto results = p.parse(diag);
      CHECK(diag.count() == 0);
      REQUIRE(results.size() == 1);
      CHECK(std::dynamic_pointer_cast<DotAlign>(results[0]));
    }
  }

  SECTION(".ASCII") {
    {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data(".ASCII \"hi\""));
      auto results = p.parse(diag);
      CHECK(diag.count() == 0);
      REQUIRE(results.size() == 1);
      CHECK(std::dynamic_pointer_cast<DotLiteral>(results[0]));
    }
    {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data(".ASCII \"\""));
      auto results = p.parse(diag);
      CHECK(diag.count() == 0);
      REQUIRE(results.size() == 1);
      CHECK(std::dynamic_pointer_cast<DotLiteral>(results[0]));
    }
  }

  SECTION(".BLOCK") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(".BLOCK 7"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    CHECK(std::dynamic_pointer_cast<DotBlock>(results[0]));
  }

  SECTION(".BYTE") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(".BYTE 255"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    CHECK(std::dynamic_pointer_cast<DotLiteral>(results[0]));
  }

  SECTION(".EQUATE") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("s: .EQUATE 10"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    CHECK(std::dynamic_pointer_cast<DotEquate>(results[0]));
  }

  SECTION(".EXPORT") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(".EXPORT charIn"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    CHECK(std::dynamic_pointer_cast<DotAnnotate>(results[0]));
  }

  SECTION(".IMPORT") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(".IMPORT charIn"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    CHECK(std::dynamic_pointer_cast<DotAnnotate>(results[0]));
  }

  SECTION(".INPUT") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(".INPUT charIn"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    CHECK(std::dynamic_pointer_cast<DotAnnotate>(results[0]));
  }

  SECTION(".OUTPUT") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(".ORG 0xfeed"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    CHECK(std::dynamic_pointer_cast<DotOrg>(results[0]));
  }

  SECTION(".OUTPUT") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(".OUTPUT charOut"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    CHECK(std::dynamic_pointer_cast<DotAnnotate>(results[0]));
  }

  SECTION(".SECTION") {
    {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data(".SECTION \".text\", \"rw\""));
      auto results = p.parse(diag);
      CHECK(diag.count() == 0);
      REQUIRE(results.size() == 1);
      auto r0 = std::dynamic_pointer_cast<DotSection>(results[0]);
      REQUIRE(r0);
      REQUIRE(r0->name.value != nullptr);
      CHECK(*r0->name.value == ".text");
      CHECK(r0->flags.r == true);
      CHECK(r0->flags.w == true);
      CHECK(r0->flags.x == false);
    }
    {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data(".SECTION \".\", \"x\""));
      auto results = p.parse(diag);
      CHECK(diag.count() == 0);
      REQUIRE(results.size() == 1);
      auto r0 = std::dynamic_pointer_cast<DotSection>(results[0]);
      REQUIRE(r0);
      CHECK(*r0->name.value == ".");
      CHECK(r0->flags.r == false);
      CHECK(r0->flags.w == false);
      CHECK(r0->flags.x == true);
    }
  }
  SECTION(".SCALL") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(".SCALL feed"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    CHECK(std::dynamic_pointer_cast<DotAnnotate>(results[0]));
  }

  SECTION(".WORD") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(".WORD 0xFFFF"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    CHECK(std::dynamic_pointer_cast<DotLiteral>(results[0]));
  }
}
