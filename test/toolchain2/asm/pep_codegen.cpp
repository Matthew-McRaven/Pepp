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
  auto sections = pepp::tc::split_to_sections(results);
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
    auto sections = pepp::tc::split_to_sections(results);
    pepp::tc::assign_addresses(sections);
    CHECK(sections.size() == 3);

    CHECK(sections[0].first.name == ".text");
    CHECK(sections[0].second.size() == 5);
    auto s0 = sections[0].second;
    CHECK(!s0[0]->attribute(attr::Address::TYPE));
    CHECK(!s0[1]->attribute(attr::Address::TYPE));
    CHECK(s0[2]->typed_attribute<attr::Address>()->address == 0);
    CHECK(!s0[3]->attribute(attr::Address::TYPE));
    CHECK(s0[4]->typed_attribute<attr::Address>()->address == 3);
    CHECK(sections[1].first.name == ".data");
    CHECK(sections[1].second.size() == 3);
    auto s1 = sections[1].second;
    CHECK(!s1[0]->attribute(attr::Address::TYPE));
    CHECK(s1[1]->typed_attribute<attr::Address>()->address == 0);
    CHECK(s1[2]->typed_attribute<attr::Address>()->address == 30);
    CHECK(sections[2].first.name == "memvec");
    CHECK(sections[2].second.size() == 3);
    auto s2 = sections[2].second;
    CHECK(!s2[0]->attribute(attr::Address::TYPE));
    CHECK(s2[1]->typed_attribute<attr::Address>()->address == 0);
    CHECK(s2[2]->typed_attribute<attr::Address>()->address == 1);
  }
}
