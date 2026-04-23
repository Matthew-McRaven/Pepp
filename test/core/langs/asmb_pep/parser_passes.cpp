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
#include "core/compile/ir_value/numeric.hpp"
#include "core/compile/symbol/entry.hpp"
#include "core/langs/asmb/diagnostic_table.hpp"
#include "core/langs/asmb_pep/ir_lines.hpp"
#include "core/langs/asmb_pep/parser.hpp"
#include "spdlog/spdlog.h"

namespace {
static auto data = [](auto str) { return pepp::tc::support::SeekableData{str}; };
} // namespace

TEST_CASE("Pepp ASM parser", "[scope:core][scope:core.langs][level:asmb3][level:asmb5][kind:unit][arch:*]") {
  using Lexer = pepp::tc::lex::PepLexer;
  using Parser = pepp::tc::parser::PepParser;
  using SymbolTable = pepp::core::symbol::LeafTable;
  using namespace pepp::tc;
  using MR = pepp::tc::MacroRegistry;
  SECTION("No input") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(" "), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    CHECK(std::dynamic_pointer_cast<EmptyLine>(results[0]));
  }
  SECTION("Empty lines") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("   \n   "), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 2);
    CHECK(std::dynamic_pointer_cast<EmptyLine>(results[0]));
    CHECK(std::dynamic_pointer_cast<EmptyLine>(results[1]));
  }
  SECTION("Monadic instructions") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("NOTA"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    auto r0 = std::dynamic_pointer_cast<MonadicInstruction>(results[0]);
    REQUIRE(r0);
    CHECK(r0->mnemonic.instruction == isa::detail::pep10::Mnemonic::NOTA);
  }
  SECTION("Monadic instructions declaring symbols") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("symb: NOTA"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    auto r0 = std::dynamic_pointer_cast<MonadicInstruction>(results[0]);
    REQUIRE(r0);
    CHECK(r0->mnemonic.instruction == isa::detail::pep10::Mnemonic::NOTA);
    auto attr = r0->attribute(SymbolDeclaration::TYPE);
    REQUIRE(attr);
    auto sym = (SymbolDeclaration *)attr;
    CHECK(sym->entry->name == "symb");
    CHECK(sym->entry->is_singly_defined());
  }
  SECTION("Dyadic instructions") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("ADDA 10,i"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    auto r0 = std::dynamic_pointer_cast<DyadicInstruction>(results[0]);
    REQUIRE(r0);
    CHECK(r0->mnemonic.instruction == isa::detail::pep10::Mnemonic::ADDA);
    CHECK(r0->addr_mode.addr_mode == isa::detail::pep10::AddressingMode::I);
    CHECK(std::dynamic_pointer_cast<pepp::ast::Numeric>(r0->argument.value));
  }
  SECTION("Dyadic instructions with optional addressing modes") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("BR 10"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    auto r0 = std::dynamic_pointer_cast<DyadicInstruction>(results[0]);
    REQUIRE(r0);
    CHECK(r0->mnemonic.instruction == isa::detail::pep10::Mnemonic::BR);
    CHECK(r0->addr_mode.addr_mode == isa::detail::pep10::AddressingMode::I);
    auto ptr_arg = std::dynamic_pointer_cast<pepp::ast::Numeric>(r0->argument.value);
    CHECK(ptr_arg != nullptr);
  }
  SECTION("Dyadic instructions with large argument") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("BR 0xffff"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    auto r0 = std::dynamic_pointer_cast<DyadicInstruction>(results[0]);
    REQUIRE(r0);
    CHECK(r0->mnemonic.instruction == isa::detail::pep10::Mnemonic::BR);
    CHECK(r0->addr_mode.addr_mode == isa::detail::pep10::AddressingMode::I);
    CHECK(std::dynamic_pointer_cast<pepp::ast::Numeric>(r0->argument.value) != nullptr);
  }
  SECTION("Dyadic instructions with symbolic argument") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("this:BR this,x"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    auto r0 = std::dynamic_pointer_cast<DyadicInstruction>(results[0]);
    REQUIRE(r0);
    CHECK(r0->mnemonic.instruction == isa::detail::pep10::Mnemonic::BR);
    CHECK(r0->addr_mode.addr_mode == isa::detail::pep10::AddressingMode::X);
    CHECK(std::dynamic_pointer_cast<pepp::ast::Symbolic>(r0->argument.value));
  }
}

TEST_CASE("Pepp ASM parser dot commands",
          "[scope:core][scope:core.langs][level:asmb3][level:asmb5][kind:unit][arch:*]") {
  using Lexer = pepp::tc::lex::PepLexer;
  using Parser = pepp::tc::parser::PepParser;
  using SymbolTable = pepp::core::symbol::LeafTable;
  using namespace pepp::tc;
  using MR = pepp::tc::MacroRegistry;

  SECTION(".ALIGN") {
    {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data(".ALIGN 1"), std::make_shared<MR>());
      auto results = p.parse(diag);
      CHECK(diag.count() == 0);
      REQUIRE(results.size() == 1);
      auto casted = std::dynamic_pointer_cast<DotAlign>(results[0]);
      CHECK(casted);
      CHECK(casted->which == DotAlign::Which::ByteCount);
    }
    {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data("s: .ALIGN 4"), std::make_shared<MR>());
      auto results = p.parse(diag);
      CHECK(diag.count() == 0);
      REQUIRE(results.size() == 1);
      auto casted = std::dynamic_pointer_cast<DotAlign>(results[0]);
      CHECK(casted);
      CHECK(casted->which == DotAlign::Which::ByteCount);
    }
  }

  SECTION(".ASCII") {
    {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data(".ASCII \"hi\""), std::make_shared<MR>());
      auto results = p.parse(diag);
      CHECK(diag.count() == 0);
      REQUIRE(results.size() == 1);
      CHECK(std::dynamic_pointer_cast<DotLiteral>(results[0]));
    }
    {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data(".ASCII \"\""), std::make_shared<MR>());
      auto results = p.parse(diag);
      CHECK(diag.count() == 0);
      REQUIRE(results.size() == 1);
      CHECK(std::dynamic_pointer_cast<DotLiteral>(results[0]));
    }
  }

  SECTION(".BLOCK") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(".BLOCK 7"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    CHECK(std::dynamic_pointer_cast<DotBlock>(results[0]));
  }

  SECTION(".BYTE") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(".BYTE 255"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    CHECK(std::dynamic_pointer_cast<DotLiteral>(results[0]));
  }

  SECTION(".BYTE0") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(".BYTE 0"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    CHECK(std::dynamic_pointer_cast<DotLiteral>(results[0]));
  }

  SECTION(".EQUATE") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("s: .EQUATE 10"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    CHECK(std::dynamic_pointer_cast<DotEquate>(results[0]));
  }

  SECTION(".EXPORT") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(".EXPORT charIn"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    CHECK(std::dynamic_pointer_cast<DotAnnotate>(results[0]));
  }

  SECTION(".IMPORT") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(".IMPORT charIn"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    CHECK(std::dynamic_pointer_cast<DotAnnotate>(results[0]));
  }

  SECTION(".INPUT") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(".INPUT charIn"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    CHECK(std::dynamic_pointer_cast<DotAnnotate>(results[0]));
  }

  SECTION(".ORG") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(".ORG 0xfeed"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    CHECK(std::dynamic_pointer_cast<DotOrg>(results[0]));
  }

  SECTION(".OUTPUT") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(".OUTPUT charOut"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    CHECK(std::dynamic_pointer_cast<DotAnnotate>(results[0]));
  }

  SECTION(".SECTION") {
    {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data(".SECTION \".text\", \"rw\""), std::make_shared<MR>());
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
      auto p = Parser(data(".SECTION \".\", \"x\""), std::make_shared<MR>());
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
  SECTION(".SCALL") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(".SCALL feed"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    CHECK(std::dynamic_pointer_cast<DotAnnotate>(results[0]));
  }

  SECTION(".WORD") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(".WORD 0xFFFF"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    CHECK(std::dynamic_pointer_cast<DotLiteral>(results[0]));
  }

  SECTION("Trivial true .IF") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(".IF 1\n.BYTE 5\n.ENDIF"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 3);
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[0]));
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[2]));
  }
  SECTION("Trivial false .IF") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(".IF 0\n.BYTE 5\n.ENDIF"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 2);
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[0]));
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[1]));
  }

  SECTION("Trivial nested true-true .IF") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(".IF 1\n.IF 1\n.BYTE 5\n.endif\n.ENDIF"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 5);
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[0]));
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[1]));
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[3]));
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[4]));
  }
  SECTION("Trivial nested true-false .IF") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(".IF 1\n .IF 0\n .BYTE 5\n .endif\n .ENDIF"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 4);
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[0]));
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[1]));
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[2]));
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[3]));
  }
  SECTION("Trivial nested false-false .IF") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(".IF 0\n .IF 0\n .BYTE 5\n .ENDIF\n .ENDIF"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 2);
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[0]));
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[1]));
  }
  SECTION("Trivial nested false-true .IF") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(".IF 0\n.IF 1\n.BYTE 5\n.ENDIF\n.ENDIF"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 2);
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[0]));
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[1]));
  }

  SECTION("Trivial false .IF + true elseif") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(".IF 0\n.BYTE 5\n.byte 5\n.ELSEIF 1\n .WORD 1\n.ENDIF"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 4);
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[0]));
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[1]));
    CHECK(std::dynamic_pointer_cast<DotLiteral>(results[2]));
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[3]));
  }
  SECTION("Trivial true .IF + true elseif") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(".IF 1\n.BYTE 5\n.ELSEIF 1\n .WORD 1\n.ENDIF"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 4);
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[0]));
    CHECK(std::dynamic_pointer_cast<DotLiteral>(results[1]));
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[2]));
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[3]));
  }
  SECTION("Ignore not-taken .ELSEIF") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(".IF 1\n.BYTE 5\n.ELSEIF 1\n .WORD 1\n.ENDIF"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 4);
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[0]));
    CHECK(std::dynamic_pointer_cast<DotLiteral>(results[1]));
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[2]));
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[3]));
  }
  SECTION("Ignore not-taken .ELSE") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(".IF 1\n.BYTE 5\n.ELSEIF 1\n .WORD 1\n.ELSE\n.WORD 5\n.ENDIF"), std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 4);
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[0]));
    CHECK(std::dynamic_pointer_cast<DotLiteral>(results[1]));
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[2]));
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[3]));
  }
  SECTION("nested ELSEIF") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(R"(.IF 0
    	.align
		.ELSEIF 1	
			.IF 0
				.align 2
			.ELSEIF 1
				.word 2
			.endif
		.ENDIF)"),
                    std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 7);
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[0]));
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[1]));
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[2]));
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[3]));
    CHECK(std::dynamic_pointer_cast<DotLiteral>(results[4]));
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[5]));
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[6]));
  }
  SECTION("nested ELSE") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(R"(.IF 0
    	.align
		.ELSEIF 0
    .ELSE	
			.IF 0
				.align 2
			.ELSEIF 0
      .ELSE
				.word 2
			.endif
		.ENDIF)"),
                    std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 9);
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[0]));
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[1]));
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[2]));
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[3]));
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[4]));
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[5]));
    CHECK(std::dynamic_pointer_cast<DotLiteral>(results[6]));
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[7]));
    CHECK(std::dynamic_pointer_cast<DotConditional>(results[8]));
  }
}

TEST_CASE("Pepp ASM parser with macros instantiations",
          "[scope:core][scope:core.langs][level:asmb3][level:asmb5][kind:unit][arch:*]") {
  using Lexer = pepp::tc::lex::PepLexer;
  using Parser = pepp::tc::parser::PepParser;
  using SymbolTable = pepp::core::symbol::LeafTable;
  using namespace pepp::tc;
  using MR = pepp::tc::MacroRegistry;
  SECTION("nullary macro") {
    pepp::tc::DiagnosticTable diag;
    auto mr = std::make_shared<MR>();
    auto macro = std::make_shared<MacroDefinition>(MacroDefinition{"@TEST", {}, ".byte 15\n"});
    mr->insert(macro);
    auto p = Parser(data("@TEST"), mr);
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    CHECK(std::dynamic_pointer_cast<MacroInstantiation>(results[0]));
  }
  SECTION("unary macro") {
    pepp::tc::DiagnosticTable diag;
    auto mr = std::make_shared<MR>();
    auto macro = std::make_shared<MacroDefinition>(MacroDefinition{"@TEST", {{.name = "feed"}}, ".byte \\feed\n"});
    mr->insert(macro);
    auto p = Parser(data("@TEST 15"), mr);
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    for (const auto &d : diag) SPDLOG_WARN("Diagnostic:  {}", d.second);
    REQUIRE(results.size() == 1);
    auto as_mi = std::dynamic_pointer_cast<MacroInstantiation>(results[0]);
    CHECK(as_mi);
    CHECK(as_mi->lines.size() == 1);
    auto as_mi_line0 = std::dynamic_pointer_cast<DotLiteral>(as_mi->lines[0]);
    REQUIRE(as_mi_line0);
    CHECK(as_mi_line0->argument.value->value_as<u16>() == 15);
  }
}

TEST_CASE("Pepp ASM parser with macros definitions",
          "[scope:core][scope:core.langs][level:asmb3][level:asmb5][kind:unit][arch:*]") {
  using Lexer = pepp::tc::lex::PepLexer;
  using Parser = pepp::tc::parser::PepParser;
  using SymbolTable = pepp::core::symbol::LeafTable;
  using namespace pepp::tc;
  using MR = pepp::tc::MacroRegistry;
  SECTION("nullary macro") {
    pepp::tc::DiagnosticTable diag;
    auto mr = std::make_shared<MR>();
    auto p = Parser(data(".macro @TEST\n.byte 0xfe\n.endm"), mr);
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    CHECK(std::dynamic_pointer_cast<InlineMacroDefinition>(results[0]));
    CHECK(mr->contains("@TEST"));
    CHECK(mr->find("@TEST")->arguments.empty());
    CHECK(mr->find("@TEST")->body == ".byte 0xfe\n");
  }
  SECTION("unary macro") {
    pepp::tc::DiagnosticTable diag;
    auto mr = std::make_shared<MR>();
    auto p = Parser(data(".macro @TEST feed\n.byte \\feed\n.endm"), mr);
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 1);
    CHECK(std::dynamic_pointer_cast<InlineMacroDefinition>(results[0]));
    CHECK(mr->contains("@TEST"));
    CHECK(mr->find("@TEST")->arguments.size() == 1);
    CHECK(mr->find("@TEST")->arguments.at(0).name == "feed");
    CHECK(mr->find("@TEST")->body == ".byte \\feed\n");
  }
}
