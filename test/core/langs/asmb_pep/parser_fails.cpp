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

#include <catch.hpp>
#include "core/langs/asmb/diagnostic_table.hpp"
#include "core/langs/asmb_pep/parser.hpp"
#include "core/langs/asmb_pep/parser_error.hpp"

namespace {
static auto data = [](auto str) { return pepp::tc::support::SeekableData{str}; };
} // namespace

TEST_CASE("Pepp ASM parser errors",
          "[scope:core][scope:core.langs][level:asmb3][level:asmb5][kind:unit][arch:*][!throws]") {
  using Lexer = pepp::tc::lex::PepLexer;
  using Parser = pepp::tc::parser::PepParser;
  using SymbolTable = pepp::core::symbol::LeafTable;
  using pepp::tc::support::Location;
  using pepp::tc::support::LocationInterval;
  using namespace pepp::tc;
  using NullaryError = pepp::tc::PepParserError::NullaryError;
  using UnaryError = pepp::tc::PepParserError::UnaryError;
  using PE = pepp::tc::PepParserError;
  using MR = pepp::tc::MacroRegistry;
  SECTION("Dyadic missing argument") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("\n\nadda"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 1);
    auto [start, end] = diag.overlapping_interval(LocationInterval(Location(2, 0), Location(2, Location::MAX)));
    CHECK(start != end);
    CHECK(start->second == PE::to_string(NullaryError::Argument_Missing));
  }
  SECTION("Dyadic argument too big") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("\nadda 0x10000,i"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 1);
    auto [start, end] = diag.overlapping_interval(LocationInterval(Location(1, 0), Location(1, Location::MAX)));
    CHECK(start != end);
    CHECK(start->second == PE::to_string(NullaryError::Argument_Exceeded2Bytes));
  }
  SECTION("Invalid integer format") {
    // TODO: would like to recognize invalid integers in lexer but reject them in parser.
    {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data("\nadda 0b10000,i"), std::make_shared<MR>());
      auto results = p.parse(diag);
      CHECK(diag.count() == 1);
      auto [start, end] = diag.overlapping_interval(LocationInterval(Location(1, 0), Location(1, Location::MAX)));
      CHECK(start != end);
      // CHECK(start->second == PE::to_string(NullaryError::Argument_InvalidIntegerFormat));
    }
    {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data("\nadda 0bo10000,i"), std::make_shared<MR>());
      auto results = p.parse(diag);
      CHECK(diag.count() == 1);
      auto [start, end] = diag.overlapping_interval(LocationInterval(Location(1, 0), Location(1, Location::MAX)));
      CHECK(start != end);
      // CHECK(start->second == PE::to_string(NullaryError::Argument_InvalidIntegerFormat));
    }
  }
  SECTION(".BYTE too big") {
    {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data("\n.BYTE 256"), std::make_shared<MR>());
      auto results = p.parse(diag);
      CHECK(diag.count() == 1);
      auto [start, end] = diag.overlapping_interval(LocationInterval(Location(1, 0), Location(1, Location::MAX)));
      CHECK(start != end);
      CHECK(start->second == PE::to_string(NullaryError::Argument_Exceeded1Byte));
    }
    {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data("\n.BYTE -129"), std::make_shared<MR>());
      auto results = p.parse(diag);
      CHECK(diag.count() == 1);
      auto [start, end] = diag.overlapping_interval(LocationInterval(Location(1, 0), Location(1, Location::MAX)));
      CHECK(start != end);
      CHECK(start->second == PE::to_string(NullaryError::Argument_Exceeded1Byte));
    }
  }
  SECTION(".ALIGN expected power-of-two") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("\n .ALIGN 3"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 1);
    auto [start, end] = diag.overlapping_interval(LocationInterval(Location(1, 0), Location(1, Location::MAX)));
    CHECK(start != end);
    CHECK(start->second == PE::to_string(NullaryError::Argument_ExpectedPowerOfTwo));
  }
  SECTION(".ASCII expected string") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("\n .ASCII 3"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 1);
    auto [start, end] = diag.overlapping_interval(LocationInterval(Location(1, 0), Location(1, Location::MAX)));
    CHECK(start != end);
    CHECK(start->second == PE::to_string(NullaryError::Argument_ExpectedString));
  }
  SECTION(".IMPORT expected identifier") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("\n .IMPORT 3"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 1);
    auto [start, end] = diag.overlapping_interval(LocationInterval(Location(1, 0), Location(1, Location::MAX)));
    CHECK(start != end);
    CHECK(start->second == PE::to_string(NullaryError::Argument_ExpectedIdentifier));
  }
  SECTION(".ORG expected hex") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("\n .ORG 3"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 1);
    auto [start, end] = diag.overlapping_interval(LocationInterval(Location(1, 0), Location(1, Location::MAX)));
    CHECK(start != end);
    CHECK(start->second == PE::to_string(NullaryError::Argument_ExpectedHex));
  }
  SECTION("Addressing mode required") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("\n adda 10"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 1);
    auto [start, end] = diag.overlapping_interval(LocationInterval(Location(1, 0), Location(1, Location::MAX)));
    CHECK(start != end);
    CHECK(start->second == PE::to_string(NullaryError::AddressingMode_Required));
  }
  SECTION("Addressing mode invalid") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("\n adda 10,j"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 1);
    auto [start, end] = diag.overlapping_interval(LocationInterval(Location(1, 0), Location(1, Location::MAX)));
    CHECK(start != end);
    CHECK(start->second == PE::to_string(NullaryError::AddressingMode_Invalid));
  }
  SECTION("Addressing mode missing") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("\n adda 10,"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 1);
    auto [start, end] = diag.overlapping_interval(LocationInterval(Location(1, 0), Location(1, Location::MAX)));
    CHECK(start != end);
    CHECK(start->second == PE::to_string(NullaryError::AddressingMode_Missing));
  }
  SECTION(".EQUATE expectes symbol declaration") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("\n .EQUATE 10,"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 1);
    auto [start, end] = diag.overlapping_interval(LocationInterval(Location(1, 0), Location(1, Location::MAX)));
    CHECK(start != end);
    CHECK(start->second == PE::to_string(NullaryError::SymbolDeclaration_Required));
  }
  SECTION(".ORG rejects symbols") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("\n abc:.ORG 0xfeed"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 1);
    auto [start, end] = diag.overlapping_interval(LocationInterval(Location(1, 0), Location(1, Location::MAX)));
    CHECK(start != end);
    CHECK(start->second == PE::to_string(NullaryError::SymbolDeclaration_Forbidden));
  }
  SECTION("Symbol too long") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("\n abcdefghij: .EQUATE 10,"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 1);
    auto [start, end] = diag.overlapping_interval(LocationInterval(Location(1, 0), Location(1, Location::MAX)));
    CHECK(start != end);
    CHECK(start->second == PE::to_string(NullaryError::SymbolDeclaration_TooLong));
  }
  SECTION(".SECTION expects string name") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("\n .SECTION 10"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 1);
    auto [start, end] = diag.overlapping_interval(LocationInterval(Location(1, 0), Location(1, Location::MAX)));
    CHECK(start != end);
    CHECK(start->second == PE::to_string(NullaryError::Section_StringName));
  }
  SECTION(".SECTION expects two args") {
    {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data("\n .SECTION \"a\""), std::make_shared<MR>());
      auto results = p.parse(diag);
      CHECK(diag.count() == 1);
      auto [start, end] = diag.overlapping_interval(LocationInterval(Location(1, 0), Location(1, Location::MAX)));
      CHECK(start != end);
      CHECK(start->second == PE::to_string(NullaryError::Section_TwoArgs));
    }
  }
  SECTION(".SECTION expects string flags") {
    {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data("\n .SECTION \"a\",10"), std::make_shared<MR>());
      auto results = p.parse(diag);
      CHECK(diag.count() == 1);
      auto [start, end] = diag.overlapping_interval(LocationInterval(Location(1, 0), Location(1, Location::MAX)));
      CHECK(start != end);
      CHECK(start->second == PE::to_string(NullaryError::Section_StringFlags));
    }
    {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data("\n .SECTION \"a\","), std::make_shared<MR>());
      auto results = p.parse(diag);
      CHECK(diag.count() == 1);
      auto [start, end] = diag.overlapping_interval(LocationInterval(Location(1, 0), Location(1, Location::MAX)));
      CHECK(start != end);
      CHECK(start->second == PE::to_string(NullaryError::Section_StringFlags));
    }
  }
  SECTION("Unterminated .IF") {
    {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data("\n.IF 0"), std::make_shared<MR>());
      auto results = p.parse(diag);
      CHECK(diag.count() == 1);
      auto [start, end] = diag.overlapping_interval(LocationInterval(Location(1, 0), Location(1, Location::MAX)));
      CHECK(start != end);
      CHECK(start->second == PE::to_string(NullaryError::Conditional_Unterminated));
    }
  }
  SECTION("Unmatched .ELSEIF") {
    {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data("\n.elseif 0"), std::make_shared<MR>());
      auto results = p.parse(diag);
      CHECK(diag.count() == 1);
      auto [start, end] = diag.overlapping_interval(LocationInterval(Location(1, 0), Location(1, Location::MAX)));
      CHECK(start != end);
      CHECK(start->second == PE::to_string(NullaryError::Conditional_UnmatchedElseif));
    }
  }
  SECTION("Unmatched .ENDIF") {
    {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data("\n.endif"), std::make_shared<MR>());
      auto results = p.parse(diag);
      CHECK(diag.count() == 1);
      auto [start, end] = diag.overlapping_interval(LocationInterval(Location(1, 0), Location(1, Location::MAX)));
      CHECK(start != end);
      CHECK(start->second == PE::to_string(NullaryError::Conditional_UnmatchedEndif));
    }
  }
  SECTION("Unterminated .macro") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("\n.macro @TEST"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 1);
    auto [start, end] = diag.overlapping_interval(LocationInterval(Location(1, 0), Location(1, Location::MAX)));
    CHECK(start != end);
    CHECK(start->second == PE::to_string(NullaryError::Macro_Unterminated));
  }
  SECTION("Unmatched .endm") {
    {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data("\n.endm"), std::make_shared<MR>());
      auto results = p.parse(diag);
      CHECK(diag.count() == 1);
      auto [start, end] = diag.overlapping_interval(LocationInterval(Location(1, 0), Location(1, Location::MAX)));
      CHECK(start != end);
      CHECK(start->second == PE::to_string(NullaryError::Macro_UnmatchedEndm));
    }
  }
  SECTION("Redefining a macro") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(".macro @TEST\n.endm\n.macro @TEST\n.endm"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 1);
    // TODO: IDK why this is sent to the wrong line.
    auto [start, end] = diag.overlapping_interval(LocationInterval(Location(0, 0), Location(1, Location::MAX)));
    CHECK(start != diag.cend());
    CHECK(start->second == PE::to_string(UnaryError::Macro_Redefinition, "@TEST"));
  }
}
