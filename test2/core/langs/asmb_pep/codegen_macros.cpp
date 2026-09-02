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
#include "core/compile/ir_linear/line_dot.hpp"
#include "core/compile/ir_linear/line_empty.hpp"
#include "core/compile/ir_linear/line_macro.hpp"
#include "core/langs/asmb/diagnostic_table.hpp"
#include "core/langs/asmb_pep/codegen.hpp"
#include "core/langs/asmb_pep/parser.hpp"
#include "core/langs/asmb_pep/text_format.hpp"
#include "spdlog/spdlog.h"

namespace {
static auto data = [](auto str) { return pepp::tc::support::SeekableData{str}; };
// First line is empty!!
static const auto ex1 = R"(
.macro HELLO amt
.BLOCK \amt
.ENDM
.macro MyWord amt
.WORD \amt
.ENDM
.macro MyByte amt
.BYTE \amt
.ENDM
.SECTION ".text", "rwx"
LDWA 10,d
.SECTION ".data", "rw"
HELLO 30
MyWord 10
.SECTION ".text", "rwx"
BR 0
.SECTION "memvec", "rw"
MyByte 1
MyByte 0
)";
} // namespace

TEST_CASE("Pepp macro ASM codegen address assignment",
          "[scope:core][scope:core.langs][level:asmb3][level:asmb5][kind:unit][arch:*]") {
  using Lexer = pepp::tc::lex::PepLexer;
  using Parser = pepp::tc::parser::PepParser;
  using SymbolTable = pepp::core::symbol::LeafTable;
  using MR = pepp::tc::MacroRegistry;
  using namespace pepp::tc;
  SECTION("No ORG") {
    pepp::tc::DiagnosticTable diag;
    auto mr = std::make_shared<MR>();
    auto p = Parser(data(ex1), mr);
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 14);
    auto code = pepp::tc::parser::flatten_macros(results);
    auto result = pepp::tc::pepp_split_to_sections(diag, code);
    CHECK(diag.count() == 0);
    for (const auto &d : diag) SPDLOG_WARN("Diagnostic:  {}", d.second);
    auto &sections = result.grouped_ir;
    auto addresses = pepp::tc::pepp_assign_addresses(sections);
    auto oc = pepp::tc::pepp_to_object_code(addresses, result.grouped_ir);
    CHECK(sections.size() == 3);
    for (const auto &l : format_listing(code, addresses, oc)) SPDLOG_WARN("Listed:  {}", l);

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
  SECTION("Move symbol into macro") {
    pepp::tc::DiagnosticTable diag;
    auto mr = std::make_shared<MR>();
    auto md = std::make_shared<MacroDefinition>();
    md->name = "test";
    md->body = ".byte 17";
    CHECK(mr->insert(md));
    auto p = Parser(data("hi:test"), mr);
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    auto macro = std::dynamic_pointer_cast<MacroInstantiation>(results[0]);
    CHECK(macro);
    CHECK(macro->has_attribute<SymbolDeclaration>());
    CHECK(macro->lines.size() == 1);
    CHECK(!macro->lines[0]->has_attribute<SymbolDeclaration>());
    auto code = pepp::tc::parser::flatten_macros(results);
    CHECK(code.size() == 1);
    auto ptr = std::dynamic_pointer_cast<pepp::tc::DotLiteral>(code[0]);
    CHECK(ptr);
    CHECK(ptr->has_attribute<SymbolDeclaration>());
  }
  SECTION("Add .block 0 to macro") {
    pepp::tc::DiagnosticTable diag;
    auto mr = std::make_shared<MR>();
    auto md = std::make_shared<MacroDefinition>();
    md->name = "test";
    md->body = "bye: .byte 17";
    CHECK(mr->insert(md));
    auto p = Parser(data("hi:test"), mr);
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    auto macro = std::dynamic_pointer_cast<MacroInstantiation>(results[0]);
    CHECK(macro);
    CHECK(macro->has_attribute<SymbolDeclaration>());
    CHECK(macro->lines.size() == 1);
    CHECK(macro->lines[0]->has_attribute<SymbolDeclaration>());
    auto code = pepp::tc::parser::flatten_macros(results);
    CHECK(code.size() == 2);
    auto ptr0 = std::dynamic_pointer_cast<pepp::tc::DotBlock>(code[0]);
    CHECK(ptr0);
    CHECK(ptr0->has_attribute<SymbolDeclaration>());
    auto ptr1 = std::dynamic_pointer_cast<pepp::tc::DotLiteral>(code[1]);
    CHECK(ptr1);
    CHECK(ptr1->has_attribute<SymbolDeclaration>());
  }
}
