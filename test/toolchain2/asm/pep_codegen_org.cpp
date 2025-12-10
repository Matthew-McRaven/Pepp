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
#include "toolchain2/asmb/pep_codegen.hpp"
#include "toolchain2/asmb/pep_parser.hpp"

using namespace Qt::StringLiterals;
namespace {
static auto data = [](auto str) { return pepp::tc::support::SeekableData{str}; };
static const auto ex1 = R"(.ORG 0xfeed
.WORD 10
.WORD 20
)";
static const auto ex2 = R"(.WORD 10
.ORG 0xfeed
.WORD 20
)";
static const auto ex3 = R"(.WORD 5
.WORD 10
.SECTION ".text2", "rwx"
.ORG 0xfeed
.WORD 20
)";
static const auto ex4 = R"(.WORD 5
.WORD 10
.SECTION ".text2", "rwx"
.BLOCK 15
.ORG 0xfeed
.WORD 20
)";
} // namespace

TEST_CASE("Pepp ASM codegen .ORG address assignment", "[scope:asm][kind:unit][arch:*][tc2]") {
  using Lexer = pepp::tc::lex::PepLexer;
  using Parser = pepp::tc::parser::PepParser;
  using SymbolTable = symbol::Table;
  using namespace pepp::tc::ir;
  SECTION("One section, .ORG at front") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(ex1));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 3);
    auto result = pepp::tc::split_to_sections(results);
    auto &sections = result.grouped_ir;
    auto addresses = pepp::tc::assign_addresses(sections);
    CHECK(sections.size() == 1);

    CHECK(sections[0].first.name == ".text");
    CHECK(sections[0].second.size() == 3);
    auto s0 = sections[0].second;
    CHECK(addresses.find(&*s0[0]) == addresses.end());
    CHECK(addresses.at(&*s0[1]).address == 0xfeed);
    CHECK(addresses.at(&*s0[2]).address == 0xfeed + 2);
  }
  SECTION("One section, .ORG in middle") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(ex2));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 3);
    auto result = pepp::tc::split_to_sections(results);
    auto &sections = result.grouped_ir;
    auto addresses = pepp::tc::assign_addresses(sections);
    CHECK(sections.size() == 1);

    CHECK(sections[0].first.name == ".text");
    CHECK(sections[0].second.size() == 3);
    auto s0 = sections[0].second;
    CHECK(addresses.at(&*s0[0]).address == 0xfeed - 2);
    CHECK(addresses.find(&*s0[1]) == addresses.end());
    CHECK(addresses.at(&*s0[2]).address == 0xfeed);
  }
  SECTION("Two sections, .ORG at start of second section") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(ex3));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 5);
    auto result = pepp::tc::split_to_sections(results);
    auto &sections = result.grouped_ir;
    auto addresses = pepp::tc::assign_addresses(sections);
    CHECK(sections.size() == 2);

    CHECK(sections[0].first.name == ".text");
    CHECK(sections[0].second.size() == 2);
    auto s0 = sections[0].second;
    CHECK(addresses.at(&*s0[0]).address == 0xfeed - 4);
    CHECK(addresses.at(&*s0[1]).address == 0xfeed - 2);

    CHECK(sections[1].first.name == ".text2");
    CHECK(sections[1].second.size() == 3);
    auto s1 = sections[1].second;
    CHECK(addresses.find(&*s1[0]) == addresses.end());
    CHECK(addresses.find(&*s1[1]) == addresses.end());
    CHECK(addresses.at(&*s1[2]).address == 0xfeed);
  }
  SECTION("Two sections, .ORG in middle of second section") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(ex4));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 6);
    auto result = pepp::tc::split_to_sections(results);
    auto &sections = result.grouped_ir;
    auto addresses = pepp::tc::assign_addresses(sections);
    CHECK(sections.size() == 2);

    CHECK(sections[0].first.name == ".text");
    CHECK(sections[0].second.size() == 2);
    auto s0 = sections[0].second;
    CHECK(addresses.at(&*s0[0]).address == 0xfeed - 19);
    CHECK(addresses.at(&*s0[1]).address == 0xfeed - 17);

    CHECK(sections[1].first.name == ".text2");
    CHECK(sections[1].second.size() == 4);
    auto s1 = sections[1].second;
    CHECK(addresses.find(&*s1[0]) == addresses.end());
    CHECK(addresses.at(&*s1[1]).address == 0xfeed - 15);
    CHECK(addresses.find(&*s1[2]) == addresses.end());
    CHECK(addresses.at(&*s1[3]).address == 0xfeed);
  }
}
