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
#include "core/langs/asmb_riscv/codegen.hpp"
#include "core/langs/asmb_riscv/parser.hpp"
#include "elfio/elfio.hpp"

namespace {
static auto data = [](auto str) { return pepp::tc::support::SeekableData{str}; };
} // namespace

TEST_CASE("RISCV ASM code generator",
          "[scope:core][scope:core.langs][level:asmb3][level:asmb5][kind:unit][arch:riscv]") {
  using Lexer = pepp::langs::RISCVLexer;
  using Parser = pepp::tc::parser::RISCVParser;
  using SymbolTable = pepp::core::symbol::LeafTable;
  using namespace pepp::tc;
  SECTION("R Type: add x1, x2, x3") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("add x1, x2, x3"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    auto result = pepp::tc::riscv_split_to_sections(diag, results);
    CHECK(diag.count() == 0);
    auto symbol_tab = p.symbol_table();
    auto &sections = result.grouped_ir;
    auto addresses = pepp::tc::riscv_assign_addresses(sections, 0xfeed);
    CHECK(addresses.container.size() == 1);
    auto instr = std::dynamic_pointer_cast<RTypeIR>(sections[0].second[0]);
    CHECK(instr != nullptr);
    CHECK(instr->mnemonic.mn == riscv::ADD);
    CHECK(addresses.count(&*instr) == 1);
    CHECK(addresses.at(&*instr).address == 0xfeed);
    CHECK(addresses.at(&*instr).size == 4);
    auto object_code = pepp::tc::riscv_to_object_code(addresses, sections);
    auto elf_result = pepp::tc::riscv_to_elf(sections, addresses, object_code);
    pepp::tc::write_symbol_table(elf_result, *symbol_tab, object_code);
    elf_result.elf->save("dummy_riscv.elf");
    CHECK(elf_result.elf->sections[".text"]->get_size() == 4);
  }
}
