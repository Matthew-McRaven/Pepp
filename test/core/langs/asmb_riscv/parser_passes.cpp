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
#include "core/compile/ir_linear/line_comment.hpp"
#include "core/compile/ir_linear/line_dot.hpp"
#include "core/compile/ir_linear/line_empty.hpp"
#include "core/compile/ir_linear/line_macro.hpp"
#include "core/compile/ir_linear/line_symbol.hpp"
#include "core/compile/ir_value/expression.hpp"
#include "core/compile/ir_value/numeric.hpp"
#include "core/compile/symbol/entry.hpp"
#include "core/compile/symbol/leaf_table.hpp"
#include "core/compile/symbol/value.hpp"
#include "core/langs/asmb/diagnostic_table.hpp"
#include "core/langs/asmb/macros.hpp"
#include "core/langs/asmb_riscv/parser.hpp"
#include "core/langs/asmb_riscv/parser_error.hpp"

namespace {
static auto data = [](auto str) { return pepp::tc::support::SeekableData{str}; };
} // namespace

TEST_CASE("RISCV ASM parser", "[scope:core][scope:core.langs][level:asmb3][level:asmb5][kind:unit][arch:riscv][!throws]") {
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
  SECTION("Immediates may be expressions") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("addi x1, x2, 4 * 3 + 1\nlw x1, 8 + 4(x3)\naddi x1, x2, -1\nl: jal ra, l + 4\njal l + 4"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 5);
    const auto imm = [&](size_t index) { return std::dynamic_pointer_cast<IntegerInstruction>(results[index])->imm; };
    CHECK(std::dynamic_pointer_cast<pepp::ast::Expression>(imm(0)));
    CHECK(imm(0)->value_as<i32>() == 13);
    CHECK(imm(0)->string() == "4 * 3 + 1");
    // An expression ends at the parenthesized register.
    CHECK(imm(1)->value_as<i32>() == 12);
    CHECK(std::dynamic_pointer_cast<ITypeIR>(results[1])->rs1 == 3);
    // A signed decimal is still parsed as before.
    CHECK(std::dynamic_pointer_cast<pepp::ast::SignedDecimal>(imm(2)));
    // Both forms of jal reach the expression, though one is tried and rolled back first.
    CHECK(imm(3)->string() == "l + 4");
    CHECK(imm(4)->string() == "l + 4");
    // A register tried as an immediate does not linger as a symbol.
    CHECK(!p.symbol_table()->exists("ra"));
    // Registers must not accidentally become symbols.
    CHECK(!p.symbol_table()->exists("x3"));
    CHECK(!p.symbol_table()->exists("x1"));
  }
  SECTION("Relocation modifiers") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("lui x1, %hi(0x12345FFF)\naddi x1, x1, %lo(0x12345FFF)"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 2);
    // %hi rounds up, since %lo's lower 12 bits are sign-extended: 0x12346000 - 1 == 0x12345FFF.
    CHECK(std::dynamic_pointer_cast<IntegerInstruction>(results[0])->imm->value_as<u32>() == 0x12346);
    CHECK(std::dynamic_pointer_cast<IntegerInstruction>(results[1])->imm->value_as<i32>() == -1);
    // Only the known modifiers may be named with a %.
    pepp::tc::DiagnosticTable unknown;
    (void)Parser(data("addi x1, x1, %nope(1)")).parse(unknown);
    CHECK(unknown.count() == 1);
  }
  SECTION("Constant expressions are checked as they are parsed") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("addi x1, x2, 1 / 0"));
    (void)p.parse(diag);
    CHECK(diag.count() == 1);
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
      auto casted = std::dynamic_pointer_cast<DotAlign>(results[0]);
      CHECK(casted);
      CHECK(casted->which == DotAlign::Which::Pow2);
    }
    {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data("s: .ALIGN 3"));
      auto results = p.parse(diag);
      CHECK(diag.count() == 0);
      REQUIRE(results.size() == 1);
      CHECK(std::dynamic_pointer_cast<DotAlign>(results[0]));
    }
  }

  SECTION(".BALIGN") {
    {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data(".BALIGN 1"));
      auto results = p.parse(diag);
      CHECK(diag.count() == 0);
      REQUIRE(results.size() == 1);
      CHECK(std::dynamic_pointer_cast<DotAlign>(results[0]));
    }
    {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data("s: .BALIGN 3"));
      auto results = p.parse(diag);
      CHECK(diag.count() == 0);
      REQUIRE(results.size() == 1);
      CHECK(std::dynamic_pointer_cast<DotAlign>(results[0]));
    }
  }

  SECTION(".P2ALIGN") {
    {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data(".P2ALIGN 1"));
      auto results = p.parse(diag);
      CHECK(diag.count() == 0);
      REQUIRE(results.size() == 1);
      CHECK(std::dynamic_pointer_cast<DotAlign>(results[0]));
    }
    {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data("s: .P2ALIGN 3"));
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
    {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data(".BLOCK 7"));
      auto results = p.parse(diag);
      CHECK(diag.count() == 0);
      REQUIRE(results.size() == 1);
      CHECK(std::dynamic_pointer_cast<DotBlock>(results[0]));
    }
    {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data(".ZERO 7"));
      auto results = p.parse(diag);
      CHECK(diag.count() == 0);
      REQUIRE(results.size() == 1);
      CHECK(std::dynamic_pointer_cast<DotBlock>(results[0]));
    }
    {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data(".skip 7"));
      auto results = p.parse(diag);
      CHECK(diag.count() == 0);
      REQUIRE(results.size() == 1);
      CHECK(std::dynamic_pointer_cast<DotBlock>(results[0]));
    }
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
    auto masked = p.symbol_table()->get("s").value()->value->value();
    CHECK(masked() == 10);
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
    {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data(".TEXT"));
      auto results = p.parse(diag);
      CHECK(diag.count() == 0);
      REQUIRE(results.size() == 1);
      auto r0 = std::dynamic_pointer_cast<DotSection>(results[0]);
      REQUIRE(r0);
      CHECK(r0->name.value == ".text");
      CHECK(r0->flags.r == true);
      CHECK(r0->flags.w == false);
      CHECK(r0->flags.x == true);
      CHECK(r0->flags.z == false);
    }
    {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data(".BSS"));
      auto results = p.parse(diag);
      CHECK(diag.count() == 0);
      REQUIRE(results.size() == 1);
      auto r0 = std::dynamic_pointer_cast<DotSection>(results[0]);
      REQUIRE(r0);
      CHECK(r0->name.value == ".bss");
      CHECK(r0->flags.r == true);
      CHECK(r0->flags.w == true);
      CHECK(r0->flags.x == false);
      CHECK(r0->flags.z == true);
    }
    {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data(".data"));
      auto results = p.parse(diag);
      CHECK(diag.count() == 0);
      REQUIRE(results.size() == 1);
      auto r0 = std::dynamic_pointer_cast<DotSection>(results[0]);
      REQUIRE(r0);
      CHECK(r0->name.value == ".data");
      CHECK(r0->flags.r == true);
      CHECK(r0->flags.w == true);
      CHECK(r0->flags.x == false);
      CHECK(r0->flags.z == false);
    }
    {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data(".rodata"));
      auto results = p.parse(diag);
      CHECK(diag.count() == 0);
      REQUIRE(results.size() == 1);
      auto r0 = std::dynamic_pointer_cast<DotSection>(results[0]);
      REQUIRE(r0);
      CHECK(r0->name.value == ".rodata");
      CHECK(r0->flags.r == true);
      CHECK(r0->flags.w == false);
      CHECK(r0->flags.x == false);
      CHECK(r0->flags.z == false);
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
  SECTION("Data and .EQUATE  allow expressions") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(".WORD 1 << 4\n.HALF 0xFF + 1\n.BYTE 'a' + 1\na: .EQUATE 2\nb: .EQUATE a * 3"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 5);
    const auto literal = [&](size_t index) {
      return std::dynamic_pointer_cast<DotLiteral>(results[index])->argument.value;
    };
    CHECK(literal(0)->value_as<u32>() == 16);
    CHECK(literal(1)->value_as<u16>() == 0x100);
    CHECK(literal(2)->value_as<u8>() == 'b');
    auto masked = p.symbol_table()->get("b").value()->value->value();
    CHECK(masked() == 6);
  }
  SECTION("Expression constraints for assembler directives") {
    // Too large for a byte; an equate naming a label, whose address is not known yet.
    for (const char *source : {".BYTE 255 + 1", "l: .WORD 0\nb: .EQUATE l + 1"}) {
      CAPTURE(source);
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data(source));
      (void)p.parse(diag);
      CHECK(diag.count() == 1);
    }
  }
  // Conditionals are shared with Pep/10, whose tests cover them in more depth.
  SECTION("Conditionals") {
    const auto parse = [](const char *source, DiagnosticTable &diag) { return Parser(data(source)).parse(diag); };
    {
      // Skipped lines (including the .else) produce no IR, and an untaken .elseif has unevaluated arguments.
      DiagnosticTable diag;
      const auto results = parse(".if 0x10000\n.byte 1\n.elseif undefined\n.byte 2\n.else\n.byte 3\n.endif", diag);
      CHECK(diag.count() == 0);
      REQUIRE(results.size() == 4);
      CHECK(std::dynamic_pointer_cast<DotConditional>(results[0]));
      CHECK(std::dynamic_pointer_cast<DotLiteral>(results[1])->argument.value->value_as<u8>() == 1);
      CHECK(std::dynamic_pointer_cast<DotConditional>(results[2]));
      CHECK(std::dynamic_pointer_cast<DotConditional>(results[3]));
    }
    {
      // A conditional nested in an untaken branch is skipped.
      DiagnosticTable diag;
      const auto results = parse("k: .equ 0\n.if k\n.if 1\n.byte 1\n.endif\n.else\nadd x1, x2, x3\n.endif", diag);
      CHECK(diag.count() == 0);
      REQUIRE(results.size() == 5);
      CHECK(std::dynamic_pointer_cast<RTypeIR>(results[3]));
    }
    using RVPE = RISCVParserError;
    for (const auto &[source, error] : std::vector<std::pair<const char *, RVPE::NullaryError>>{
             {".if 0", RVPE::NullaryError::Conditional_Unterminated},
             {".endif", RVPE::NullaryError::Conditional_UnmatchedEndif},
             {".if 0\n.else\n.else\n.endif", RVPE::NullaryError::Conditional_MultipleElse},
             {"x: .word 0\n.if x", RVPE::NullaryError::Conditional_NotConstant},
         }) {
      CAPTURE(source);
      DiagnosticTable diag;
      (void)parse(source, diag);
      REQUIRE(diag.count() == 1);
      CHECK(diag.cbegin()->second == RVPE::to_string(error));
    }
  }
  // Macros are shared with Pep/10, whose tests cover them in depth.
  SECTION("Macros") {
    const auto parse = [](const char *source, DiagnosticTable &diag) { return Parser(data(source)).parse(diag); };
    {
      // Arguments are substituted and symbol declaration moves into body. The parser owns the symbol names.
      DiagnosticTable diag;
      auto p = Parser(data(".macro inc reg\naddi \\reg, \\reg, 1\n.endm\ntop: inc x5"));
      const auto results = p.parse(diag);
      CHECK(diag.count() == 0);
      REQUIRE(results.size() == 2);
      CHECK(std::dynamic_pointer_cast<InlineMacroDefinition>(results[0]));
      const auto instantiation = std::dynamic_pointer_cast<MacroInstantiation>(results[1]);
      REQUIRE(instantiation);
      CHECK(instantiation->arguments == std::vector<std::string>{"x5"});
      const auto flattened = parser::flatten_macros(results);
      REQUIRE(flattened.size() == 2);
      const auto symbol = std::dynamic_pointer_cast<SymbolLine>(flattened[0]);
      REQUIRE(symbol);
      CHECK(symbol->symbol.entry->name == "top");
      const auto addi = std::dynamic_pointer_cast<ITypeIR>(flattened[1]);
      REQUIRE(addi);
      CHECK(addi->rd == 5);
      CHECK(addi->rs1 == 5);
    }
    using RVPE = RISCVParserError;
    for (const auto &[source, error] : std::vector<std::pair<const char *, RVPE::NullaryError>>{
             {".macro m\nm\n.endm\nm", RVPE::NullaryError::Macro_ExcessiveRecursion},
             {".macro m\nadd x1, x2, x3", RVPE::NullaryError::Macro_Unterminated},
             {".endm", RVPE::NullaryError::Macro_UnmatchedEndm},
         }) {
      CAPTURE(source);
      DiagnosticTable diag;
      (void)parse(source, diag);
      REQUIRE(diag.count() == 1);
      CHECK(diag.cbegin()->second == RVPE::to_string(error));
    }
  }
}

TEST_CASE("RISCV ASM parser symbol-only lines",
          "[scope:core][scope:core.langs][level:asmb3][level:asmb5][kind:unit][arch:riscv][!throws]") {
  using Parser = pepp::tc::parser::RISCVParser;
  using namespace pepp::tc;
  pepp::tc::DiagnosticTable diag;
  // Symbol-only lines (with and without comments) interleaved with other line types.
  auto p = Parser(data("a: # first\n"
                       "b:\n"
                       "# note\n"
                       "\n"
                       "c: add x1, x2, x3\n"
                       "d:\n"
                       ".WORD 0xFEEDBEEF\n"
                       "e:"));
  auto results = p.parse(diag);
  CHECK(diag.count() == 0);
  REQUIRE(results.size() == 8);
  const auto symbol_line = [&](size_t index, const char *name) {
    auto line = std::dynamic_pointer_cast<SymbolLine>(results[index]);
    REQUIRE(line);
    CHECK(line->symbol.entry->name == name);
    return line;
  };
  CHECK(symbol_line(0, "a")->typed_attribute<Comment>());
  CHECK(!symbol_line(1, "b")->typed_attribute<Comment>());
  CHECK(std::dynamic_pointer_cast<CommentLine>(results[2]));
  CHECK(std::dynamic_pointer_cast<EmptyLine>(results[3]));
  REQUIRE(std::dynamic_pointer_cast<RTypeIR>(results[4]));
  CHECK(results[4]->typed_attribute<SymbolDeclaration>()->entry->name == "c");
  symbol_line(5, "d");
  REQUIRE(std::dynamic_pointer_cast<DotLiteral>(results[6]));
  // A symbol-only line's symbol does not belong to the following line.
  CHECK(!results[6]->typed_attribute<SymbolDeclaration>());
  symbol_line(7, "e");
}
