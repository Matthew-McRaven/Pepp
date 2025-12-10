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
#include "toolchain2/asmb/pep_parser.hpp"

using namespace Qt::StringLiterals;
namespace {
static auto data = [](auto str) { return pepp::tc::support::SeekableData{str}; };
} // namespace

TEST_CASE("Pepp ASM parser errors", "[scope:asm][kind:unit][arch:*][tc2][!throws]") {
  using Lexer = pepp::tc::lex::PepLexer;
  using Parser = pepp::tc::parser::PepParser;
  using SymbolTable = symbol::Table;
  using pepp::tc::support::Location;
  using pepp::tc::support::LocationInterval;
  using namespace pepp::tc::ir;
  using NullaryError = pepp::tc::ParserError::NullaryError;
  using UnaryError = pepp::tc::ParserError::UnaryError;
  using PE = pepp::tc::ParserError;
  SECTION("Dyadic missing argument") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("\n\nadda"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 1);
    auto [start, end] = diag.overlapping_interval(LocationInterval(Location(2, 0), Location(2, Location::MAX)));
    CHECK(start != end);
    CHECK(start->second == PE::to_string(NullaryError::Argument_Missing));
  }
  SECTION("Dyadic argument too big") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("\nadda 0x10000,i"));
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
      auto p = Parser(data("\nadda 0b10000,i"));
      auto results = p.parse(diag);
      CHECK(diag.count() == 1);
      auto [start, end] = diag.overlapping_interval(LocationInterval(Location(1, 0), Location(1, Location::MAX)));
      CHECK(start != end);
      // CHECK(start->second == PE::to_string(NullaryError::Argument_InvalidIntegerFormat));
    }
    {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data("\nadda 0bo10000,i"));
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
      auto p = Parser(data("\n.BYTE 256"));
      auto results = p.parse(diag);
      CHECK(diag.count() == 1);
      auto [start, end] = diag.overlapping_interval(LocationInterval(Location(1, 0), Location(1, Location::MAX)));
      CHECK(start != end);
      CHECK(start->second == PE::to_string(NullaryError::Argument_Exceeded1Byte));
    }
    {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data("\n.BYTE -129"));
      auto results = p.parse(diag);
      CHECK(diag.count() == 1);
      auto [start, end] = diag.overlapping_interval(LocationInterval(Location(1, 0), Location(1, Location::MAX)));
      CHECK(start != end);
      CHECK(start->second == PE::to_string(NullaryError::Argument_Exceeded1Byte));
    }
  }
  SECTION(".ALIGN expected power-of-two") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("\n .ALIGN 3"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 1);
    auto [start, end] = diag.overlapping_interval(LocationInterval(Location(1, 0), Location(1, Location::MAX)));
    CHECK(start != end);
    CHECK(start->second == PE::to_string(NullaryError::Argument_ExpectedPowerOfTwo));
  }
  SECTION(".ASCII expected string") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("\n .ASCII 3"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 1);
    auto [start, end] = diag.overlapping_interval(LocationInterval(Location(1, 0), Location(1, Location::MAX)));
    CHECK(start != end);
    CHECK(start->second == PE::to_string(NullaryError::Argument_ExpectedString));
  }
  SECTION(".IMPORT expected identifier") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("\n .IMPORT 3"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 1);
    auto [start, end] = diag.overlapping_interval(LocationInterval(Location(1, 0), Location(1, Location::MAX)));
    CHECK(start != end);
    CHECK(start->second == PE::to_string(NullaryError::Argument_ExpectedIdentifier));
  }
  SECTION(".ORG expected hex") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("\n .ORG 3"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 1);
    auto [start, end] = diag.overlapping_interval(LocationInterval(Location(1, 0), Location(1, Location::MAX)));
    CHECK(start != end);
    CHECK(start->second == PE::to_string(NullaryError::Argument_ExpectedHex));
  }
  SECTION("Addressing mode required") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("\n adda 10"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 1);
    auto [start, end] = diag.overlapping_interval(LocationInterval(Location(1, 0), Location(1, Location::MAX)));
    CHECK(start != end);
    CHECK(start->second == PE::to_string(NullaryError::AddressingMode_Required));
  }
  SECTION("Addressing mode invalid") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("\n adda 10,j"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 1);
    auto [start, end] = diag.overlapping_interval(LocationInterval(Location(1, 0), Location(1, Location::MAX)));
    CHECK(start != end);
    CHECK(start->second == PE::to_string(NullaryError::AddressingMode_Invalid));
  }
  SECTION("Addressing mode missing") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("\n adda 10,"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 1);
    auto [start, end] = diag.overlapping_interval(LocationInterval(Location(1, 0), Location(1, Location::MAX)));
    CHECK(start != end);
    CHECK(start->second == PE::to_string(NullaryError::AddressingMode_Missing));
  }
  SECTION(".EQUATE expectes symbol declaration") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("\n .EQUATE 10,"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 1);
    auto [start, end] = diag.overlapping_interval(LocationInterval(Location(1, 0), Location(1, Location::MAX)));
    CHECK(start != end);
    CHECK(start->second == PE::to_string(NullaryError::SymbolDeclaration_Required));
  }
  SECTION(".ORG rejects symbols") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("\n abc:.ORG 0xfeed"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 1);
    auto [start, end] = diag.overlapping_interval(LocationInterval(Location(1, 0), Location(1, Location::MAX)));
    CHECK(start != end);
    CHECK(start->second == PE::to_string(NullaryError::SymbolDeclaration_Forbidden));
  }
  SECTION("Symbol too long") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("\n abcdefghij: .EQUATE 10,"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 1);
    auto [start, end] = diag.overlapping_interval(LocationInterval(Location(1, 0), Location(1, Location::MAX)));
    CHECK(start != end);
    CHECK(start->second == PE::to_string(NullaryError::SymbolDeclaration_TooLong));
  }
  SECTION(".SECTION expects string name") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("\n .SECTION 10"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 1);
    auto [start, end] = diag.overlapping_interval(LocationInterval(Location(1, 0), Location(1, Location::MAX)));
    CHECK(start != end);
    CHECK(start->second == PE::to_string(NullaryError::Section_StringName));
  }
  SECTION(".SECTION expects two args") {
    {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data("\n .SECTION \"a\""));
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
      auto p = Parser(data("\n .SECTION \"a\",10"));
      auto results = p.parse(diag);
      CHECK(diag.count() == 1);
      auto [start, end] = diag.overlapping_interval(LocationInterval(Location(1, 0), Location(1, Location::MAX)));
      CHECK(start != end);
      CHECK(start->second == PE::to_string(NullaryError::Section_StringFlags));
    }
    {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data("\n .SECTION \"a\","));
      auto results = p.parse(diag);
      CHECK(diag.count() == 1);
      auto [start, end] = diag.overlapping_interval(LocationInterval(Location(1, 0), Location(1, Location::MAX)));
      CHECK(start != end);
      CHECK(start->second == PE::to_string(NullaryError::Section_StringFlags));
    }
  }
}
