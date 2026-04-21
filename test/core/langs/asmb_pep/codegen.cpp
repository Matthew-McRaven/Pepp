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

#include "core/langs/asmb_pep/codegen.hpp"
#include <catch.hpp>
#include "core/compile/ir_linear/line_dot.hpp"
#include "core/compile/ir_linear/line_empty.hpp"
#include "core/langs/asmb/diagnostic_table.hpp"
#include "core/langs/asmb_pep/parser.hpp"
#include "toolchain/link/mmio.hpp"

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

TEST_CASE("Pepp ASM codegen sectioning",
          "[scope:core][scope:core.langs][level:asmb3][level:asmb5][kind:unit][arch:*]") {
  using Lexer = pepp::tc::lex::PepLexer;
  using Parser = pepp::tc::parser::PepParser;
  using SymbolTable = pepp::core::symbol::LeafTable;
  using namespace pepp::tc;
  using MR = pepp::tc::MacroRegistry;
  pepp::tc::DiagnosticTable diag;
  auto p = Parser(data(ex1), std::make_shared<MR>());
  auto results = p.parse(diag);
  CHECK(diag.count() == 0);
  REQUIRE(results.size() == 11);
  CHECK(std::dynamic_pointer_cast<EmptyLine>(results[0]));
  CHECK(std::dynamic_pointer_cast<DotSection>(results[1]));
  CHECK(std::dynamic_pointer_cast<DotSection>(results[3]));
  CHECK(std::dynamic_pointer_cast<DotSection>(results[6]));
  CHECK(std::dynamic_pointer_cast<DotSection>(results[8]));
  auto result = pepp::tc::pepp_split_to_sections(diag, results);
  CHECK(diag.count() == 0);
  auto &sections = result.grouped_ir;
  CHECK(sections.size() == 3);
  CHECK(sections[0].first.name == ".text");
  CHECK(sections[0].second.size() == 5);
  CHECK(sections[1].first.name == ".data");
  CHECK(sections[1].second.size() == 3);
  CHECK(sections[2].first.name == "memvec");
  CHECK(sections[2].second.size() == 3);
}

TEST_CASE("Pepp ASM codegen address assignment",
          "[scope:core][scope:core.langs][level:asmb3][level:asmb5][kind:unit][arch:*]") {
  using Lexer = pepp::tc::lex::PepLexer;
  using Parser = pepp::tc::parser::PepParser;
  using SymbolTable = pepp::core::symbol::LeafTable;
  using MR = pepp::tc::MacroRegistry;
  using namespace pepp::tc;
  SECTION("No ORG") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(ex1), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 11);
    CHECK(std::dynamic_pointer_cast<EmptyLine>(results[0]));
    CHECK(std::dynamic_pointer_cast<DotSection>(results[1]));
    CHECK(std::dynamic_pointer_cast<DotSection>(results[3]));
    CHECK(std::dynamic_pointer_cast<DotSection>(results[6]));
    CHECK(std::dynamic_pointer_cast<DotSection>(results[8]));
    auto result = pepp::tc::pepp_split_to_sections(diag, results);
    CHECK(diag.count() == 0);
    auto &sections = result.grouped_ir;
    auto addresses = pepp::tc::pepp_assign_addresses(sections);
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

TEST_CASE("Pepp ASM codegen .SCALL", "[scope:core][scope:core.langs][level:asmb3][level:asmb5][kind:unit][arch:*]") {
  using Lexer = pepp::tc::lex::PepLexer;
  using Parser = pepp::tc::parser::PepParser;
  using SymbolTable = pepp::core::symbol::LeafTable;
  using MR = pepp::tc::MacroRegistry;
  using namespace pepp::tc;

  pepp::tc::DiagnosticTable diag;
  auto p = Parser(data(R"(
    .SCALL DECI
    .scall deco)"),
                  std::make_shared<MR>());
  auto results = p.parse(diag);
  CHECK(diag.count() == 0);
  REQUIRE(results.size() == 3);
  auto result = pepp::tc::pepp_split_to_sections(diag, results);
  CHECK(diag.count() == 0);
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

TEST_CASE("Pepp ASM codegen .INPUT and .OUTPUT",
          "[scope:core][scope:core.langs][level:asmb3][level:asmb5][kind:unit][arch:*]") {
  using Lexer = pepp::tc::lex::PepLexer;
  using Parser = pepp::tc::parser::PepParser;
  using SymbolTable = pepp::core::symbol::LeafTable;
  using MR = pepp::tc::MacroRegistry;
  using namespace pepp::tc;

  pepp::tc::DiagnosticTable diag;
  auto p = Parser(data(R"(
    .INPUT DECI
    .OUTPUT deco)"),
                  std::make_shared<MR>());
  auto results = p.parse(diag);
  CHECK(diag.count() == 0);
  REQUIRE(results.size() == 3);
  auto result = pepp::tc::pepp_split_to_sections(diag, results);
  CHECK(diag.count() == 0);
  auto &mmios = result.mmios;
  CHECK(mmios.size() == 2);
  CHECK(mmios[0].name == "DECI");
  CHECK(mmios[0].type == obj::IO::Type::kInput);
  CHECK(mmios[1].name == "deco");
  CHECK(mmios[1].type == obj::IO::Type::kOutput);
}
