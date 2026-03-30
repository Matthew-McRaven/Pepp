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
#include "core/arch/riscv/asmb/rvi_patterns.hpp"
#include "core/compile/ir_linear/line_empty.hpp"
#include "core/compile/symbol/entry.hpp"
#include "core/langs/asmb/diagnostic_table.hpp"
#include "core/langs/asmb_riscv/parser.hpp"

namespace {
static auto data = [](auto str) { return pepp::tc::support::SeekableData{str}; };
} // namespace

TEST_CASE("RISCV ASM parser", "[scope:core][scope:core.langs][level:asmb3][level:asmb5][kind:unit][arch:riscv]") {
  using Lexer = pepp::langs::RISCVLexer;
  using Parser = pepp::tc::parser::RISCVParser;
  using SymbolTable = pepp::core::symbol::LeafTable;
  using namespace pepp::tc;
  SECTION("No input") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(" "));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    CHECK(std::dynamic_pointer_cast<EmptyLine>(results[0]));
  }
  SECTION("R Type: add x1, x2, x3") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("add x1, x2, x3"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    auto as_r = std::dynamic_pointer_cast<RTypeIR>(results[0]);
    CHECK(as_r);
    CHECK(as_r->mnemonic.mn == riscv::ADD);
    CHECK(as_r->rd == 1);
    CHECK(as_r->rs1 == 2);
    CHECK(as_r->rs2 == 3);
  }
  SECTION("I Type: lw x1, 0(x3)") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("lw x1, 0(x3)"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    auto as_i = std::dynamic_pointer_cast<ITypeIR>(results[0]);
    CHECK(as_i);
    CHECK(as_i->mnemonic.mn == riscv::LW);
    CHECK(as_i->rd == 1);
    CHECK(as_i->rs1 == 3);
    CHECK(as_i->imm);
    CHECK(as_i->imm->value_as<u32>() == 0);
  }
  SECTION("I Type: addi x1, x7, 0xfeed") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("addi x1, x7, 0xfeed"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    auto as_i = std::dynamic_pointer_cast<ITypeIR>(results[0]);
    CHECK(as_i);
    CHECK(as_i->mnemonic.mn == riscv::ADDI);
    CHECK(as_i->rd == 1);
    CHECK(as_i->rs1 == 7);
    CHECK(as_i->imm);
    CHECK(as_i->imm->value_as<u32>() == 0xfeed);
  }
  SECTION("S Type: sw x1, 0(x3)") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("sw x1, 0xfeed(x3)"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    auto as_i = std::dynamic_pointer_cast<STypeIR>(results[0]);
    CHECK(as_i);
    CHECK(as_i->mnemonic.mn == riscv::SW);
    CHECK(as_i->rs2 == 1);
    CHECK(as_i->rs1 == 3);
    CHECK(as_i->imm);
    CHECK(as_i->imm->value_as<u32>() == 0xfeed);
  }
  SECTION("B Type: beq x5, x7, 15") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("beq x5, x7, 15"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    auto as_i = std::dynamic_pointer_cast<BTypeIR>(results[0]);
    CHECK(as_i);
    CHECK(as_i->mnemonic.mn == riscv::BEQ);
    CHECK(as_i->rs1 == 5);
    CHECK(as_i->rs2 == 7);
    CHECK(as_i->imm);
    CHECK(as_i->imm->value_as<u32>() == 15);
  }
}
