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
#include "core/compile/ir_linear/line_dot.hpp"
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
    auto as_s = std::dynamic_pointer_cast<STypeIR>(results[0]);
    CHECK(as_s);
    CHECK(as_s->mnemonic.mn == riscv::SW);
    CHECK(as_s->rs2 == 1);
    CHECK(as_s->rs1 == 3);
    CHECK(as_s->imm);
    CHECK(as_s->imm->value_as<u32>() == 0xfeed);
  }
  SECTION("B Type: BEQ x5, x7, 15") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("BEQ x5, x7, 15"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    auto as_b = std::dynamic_pointer_cast<BTypeIR>(results[0]);
    CHECK(as_b);
    CHECK(as_b->mnemonic.mn == riscv::BEQ);
    CHECK(as_b->rs1 == 5);
    CHECK(as_b->rs2 == 7);
    CHECK(as_b->imm);
    CHECK(as_b->imm->value_as<u32>() == 15);
  }
  SECTION("J Type: JaL x31, -72") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("JaL x31, -72"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    auto as_j = std::dynamic_pointer_cast<JTypeIR>(results[0]);
    CHECK(as_j);
    CHECK(as_j->mnemonic.mn == riscv::JAL);
    CHECK(as_j->rd == 31);
    CHECK(as_j->imm);
    CHECK(as_j->imm->value_as<i32>() == -72);
  }
  SECTION("U Type: auipc x31, 0xcafe") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("auipc x31, 0xcafe"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    auto as_u = std::dynamic_pointer_cast<UTypeIR>(results[0]);
    CHECK(as_u);
    CHECK(as_u->mnemonic.mn == riscv::AUIPC);
    CHECK(as_u->rd == 31);
    CHECK(as_u->imm);
    CHECK(as_u->imm->value_as<u32>() == 0xcafe);
  }
}

TEST_CASE("RISCV ASM parser dot commands",
          "[scope:core][scope:core.langs][level:asmb3][level:asmb5][kind:unit][arch:*]") {
  using Lexer = pepp::langs::RISCVLexer;
  using Parser = pepp::tc::parser::RISCVParser;
  using SymbolTable = pepp::core::symbol::LeafTable;
  using namespace pepp::tc;

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

  SECTION(".ASCIZ") {
    {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data(".ASCIZ \"hi\""));
      auto results = p.parse(diag);
      CHECK(diag.count() == 0);
      REQUIRE(results.size() == 1);
      CHECK(std::dynamic_pointer_cast<DotLiteral>(results[0]));
    }
    {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data(".STRING \"\""));
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

  SECTION(".BYTE 0") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(".BYTE 0"));
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

  SECTION(".HALF") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(".HALF 0xFFFF"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    CHECK(std::dynamic_pointer_cast<DotLiteral>(results[0]));
  }

  SECTION(".ORG") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(".ORG 0xfeed"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    CHECK(std::dynamic_pointer_cast<DotOrg>(results[0]));
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
      CHECK(r0->name.value == ".text");
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
      CHECK(r0->name.value == ".");
      CHECK(r0->flags.r == false);
      CHECK(r0->flags.w == false);
      CHECK(r0->flags.x == true);
    }
  }

  SECTION(".WORD") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(".WORD 0xFEEDBEEF"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    CHECK(std::dynamic_pointer_cast<DotLiteral>(results[0]));
  }
}
