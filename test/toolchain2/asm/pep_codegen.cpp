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

#include "toolchain2/asmb/pep_codegen.hpp"
#include <catch.hpp>
#include "toolchain/link/mmio.hpp"
#include "toolchain2/asmb/pep_parser.hpp"

using namespace Qt::StringLiterals;
namespace {
static auto data = [](auto str) { return pepp::tc::support::SeekableData{str}; };
// First line is empty!!
static const auto ex1 = R"(
.SECTION ".text", "rwx"
LDWA 10,d
.SECTION ".data", "rw"
.block 30
.word 10
.SECTION ".text", "rwx"
BR 0
.SECTION "memvec", "rw"
.BYTE 0
.BYTE 0
)";
} // namespace

TEST_CASE("Pepp ASM codegen sectioning", "[scope:asm][kind:unit][arch:*][tc2]") {
  using Lexer = pepp::tc::lex::PepLexer;
  using Parser = pepp::tc::parser::PepParser;
  using SymbolTable = symbol::Table;
  using namespace pepp::tc::ir;
  auto p = Parser(data(ex1));
  auto results = p.parse();
  REQUIRE(results.size() == 11);
  CHECK(std::dynamic_pointer_cast<EmptyLine>(results[0]));
  CHECK(std::dynamic_pointer_cast<DotSection>(results[1]));
  CHECK(std::dynamic_pointer_cast<DotSection>(results[3]));
  CHECK(std::dynamic_pointer_cast<DotSection>(results[6]));
  CHECK(std::dynamic_pointer_cast<DotSection>(results[8]));
  auto result = pepp::tc::split_to_sections(results);
  auto &sections = result.grouped_ir;
  CHECK(sections.size() == 3);
  CHECK(sections[0].first.name == ".text");
  CHECK(sections[0].second.size() == 5);
  CHECK(sections[1].first.name == ".data");
  CHECK(sections[1].second.size() == 3);
  CHECK(sections[2].first.name == "memvec");
  CHECK(sections[2].second.size() == 3);
}

TEST_CASE("Pepp ASM codegen address assignment", "[scope:asm][kind:unit][arch:*][tc2]") {
  using Lexer = pepp::tc::lex::PepLexer;
  using Parser = pepp::tc::parser::PepParser;
  using SymbolTable = symbol::Table;
  using namespace pepp::tc::ir;
  SECTION("No ORG") {
    auto p = Parser(data(ex1));
    auto results = p.parse();
    REQUIRE(results.size() == 11);
    CHECK(std::dynamic_pointer_cast<EmptyLine>(results[0]));
    CHECK(std::dynamic_pointer_cast<DotSection>(results[1]));
    CHECK(std::dynamic_pointer_cast<DotSection>(results[3]));
    CHECK(std::dynamic_pointer_cast<DotSection>(results[6]));
    CHECK(std::dynamic_pointer_cast<DotSection>(results[8]));
    auto result = pepp::tc::split_to_sections(results);
    auto &sections = result.grouped_ir;
    auto addresses = pepp::tc::assign_addresses(sections);
    CHECK(sections.size() == 3);

    CHECK(sections[0].first.name == ".text");
    CHECK(sections[0].second.size() == 5);
    auto s0 = sections[0].second;
    CHECK(addresses.find(&*s0[0]) == addresses.end());
    CHECK(addresses.find(&*s0[1]) == addresses.end());
    CHECK(addresses.at(&*s0[2]).address == 0);
    CHECK(addresses.find(&*s0[3]) == addresses.end());
    CHECK(addresses.at(&*s0[4]).address == 3);
    CHECK(sections[1].first.name == ".data");
    CHECK(sections[1].second.size() == 3);
    auto s1 = sections[1].second;
    CHECK(addresses.find(&*s1[0]) == addresses.end());
    CHECK(addresses.at(&*s1[1]).address == 6);
    CHECK(addresses.at(&*s1[2]).address == 36);
    CHECK(sections[2].first.name == "memvec");
    CHECK(sections[2].second.size() == 3);
    auto s2 = sections[2].second;
    CHECK(addresses.find(&*s2[0]) == addresses.end());
    CHECK(addresses.at(&*s2[1]).address == 38);
    CHECK(addresses.at(&*s2[2]).address == 39);
  }
}

TEST_CASE("Pepp ASM codegen .SCALL", "[scope:asm][kind:unit][arch:*][tc2]") {
  using Lexer = pepp::tc::lex::PepLexer;
  using Parser = pepp::tc::parser::PepParser;
  using SymbolTable = symbol::Table;
  using namespace pepp::tc::ir;

  auto p = Parser(data(R"(
    .SCALL DECI
    .scall deco)"));
  auto results = p.parse();
  REQUIRE(results.size() == 3);
  auto result = pepp::tc::split_to_sections(results);
  auto const &scalls = result.system_calls;
  CHECK(scalls.size() == 2);
  const auto contains = [&](const std::string &target) {
    auto t = std::find(scalls.cbegin(), scalls.cend(), target);
    return t != scalls.cend();
  };
  CHECK(contains("DECI"));
  CHECK(!contains("deci"));
  CHECK(contains("deco"));
  CHECK(!contains("DECO"));
}

TEST_CASE("Pepp ASM codegen .INPUT and .OUTPUT", "[scope:asm][kind:unit][arch:*][tc2]") {
  using Lexer = pepp::tc::lex::PepLexer;
  using Parser = pepp::tc::parser::PepParser;
  using SymbolTable = symbol::Table;
  using namespace pepp::tc::ir;

  auto p = Parser(data(R"(
    .INPUT DECI
    .OUTPUT deco)"));
  auto results = p.parse();
  REQUIRE(results.size() == 3);
  auto result = pepp::tc::split_to_sections(results);
  auto &mmios = result.mmios;
  CHECK(mmios.size() == 2);
  CHECK(mmios[0].name.toStdString() == "DECI");
  CHECK(mmios[0].type == obj::IO::Type::kInput);
  CHECK(mmios[1].name.toStdString() == "deco");
  CHECK(mmios[1].type == obj::IO::Type::kOutput);
}
