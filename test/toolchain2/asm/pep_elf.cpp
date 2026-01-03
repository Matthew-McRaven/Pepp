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
#include <elfio/elfio.hpp>
#include "toolchain2/asmb/pep_codegen.hpp"
#include "toolchain2/asmb/pep_ir_visitor.hpp"
#include "toolchain2/asmb/pep_parser.hpp"

using namespace Qt::StringLiterals;
namespace {
static auto data = [](auto str) { return pepp::tc::support::SeekableData{str}; };
// First line is empty!!
static const auto ex1 = R"(
.SECTION ".text", "rwx"
bye:LDWA 10,d
.SECTION ".data", "rw"
world:.block 30
hi:.word 10
.SECTION ".text", "rwx"
cruel:BR 0
.SECTION "memvec", "rw"
World:.BYTE 0
.BYTE 0
)";
} // namespace

TEST_CASE("Pepp ASM codegen elf", "[scope:asm][kind:unit][arch:*][tc2]") {
  using Lexer = pepp::tc::lex::PepLexer;
  using Parser = pepp::tc::parser::PepParser;
  using SymbolTable = symbol::Table;
  using namespace pepp::tc::ir;
  SECTION("No ORG") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(ex1));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    REQUIRE(results.size() == 11);
    CHECK(std::dynamic_pointer_cast<EmptyLine>(results[0]));
    CHECK(std::dynamic_pointer_cast<DotSection>(results[1]));
    CHECK(std::dynamic_pointer_cast<DotSection>(results[3]));
    CHECK(std::dynamic_pointer_cast<DotSection>(results[6]));
    CHECK(std::dynamic_pointer_cast<DotSection>(results[8]));
    auto result = pepp::tc::split_to_sections(diag, results);
    CHECK(diag.count() == 0);
    auto symbol_tab = p.symbol_table();
    auto &sections = result.grouped_ir;
    auto addresses = pepp::tc::assign_addresses(sections);
    auto object_code = pepp::tc::to_object_code(addresses, sections);
    auto elf_result = pepp::tc::to_elf(sections, addresses, object_code, result.mmios);
    pepp::tc::write_symbol_table(elf_result, *symbol_tab, object_code);

    CHECK(sections.size() == 3);
    elf_result.elf->save("dummy.elf");
  }
  SECTION("0-sized section") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(R"(
      .SECTION ".data","rwx"
      test:BR 10,i)"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    auto result = pepp::tc::split_to_sections(diag, results);
    CHECK(diag.count() == 0);
    auto symbol_tab = p.symbol_table();
    auto &sections = result.grouped_ir;
    auto addresses = pepp::tc::assign_addresses(sections);
    auto object_code = pepp::tc::to_object_code(addresses, sections);
    auto elf_result = pepp::tc::to_elf(sections, addresses, object_code, result.mmios);
    pepp::tc::write_symbol_table(elf_result, *symbol_tab, object_code);

    CHECK(sections.size() == 2);
    elf_result.elf->save("dummy2.elf");
  }
  SECTION("With undefined symbols") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(R"(
			a:.BLOCK 2
			.EXPORT a
			LDWA i,i
      .BYTE i
      .WORD d
		  .SECTION ".data","rwx"
			LDWA a,i
			LDWA d,d
)"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    auto result = pepp::tc::split_to_sections(diag, results);
    CHECK(diag.count() == 0);
    auto symbol_tab = p.symbol_table();
    auto &sections = result.grouped_ir;
    auto addresses = pepp::tc::assign_addresses(sections);
    auto object_code = pepp::tc::to_object_code(addresses, sections);
    auto elf_result = pepp::tc::to_elf(sections, addresses, object_code, result.mmios);
    pepp::tc::write_symbol_table(elf_result, *symbol_tab, object_code);
    CHECK(object_code.relocations.size() == 4);
    elf_result.elf->save("needs_rel.elf");
    auto elf = elf_result.elf.get();
    REQUIRE(elf->sections.size() == 9);
    auto symtab = elf->sections[6];
    CHECK(symtab->get_name() == ".symtab");
    auto symtab_ac = ELFIO::symbol_section_accessor(*elf, symtab);
    auto rel_text = elf->sections[7];
    CHECK(rel_text->get_name() == ".rel.text");
    auto rel_data = elf->sections[8];
    CHECK(rel_data->get_name() == ".rel.data");
    auto rel_text_ac = ELFIO::relocation_section_accessor(*elf, rel_text);
    auto rel_data_ac = ELFIO::relocation_section_accessor(*elf, rel_data);
    ELFIO::Elf64_Addr rel_offset;
    ELFIO::Elf_Word rel_symbol;
    unsigned rel_type;
    ELFIO::Elf_Sxword unused;
    std::string sym_name;
    ELFIO::Elf64_Addr sym_value;
    ELFIO::Elf_Xword sym_size;
    unsigned char sym_bind, sym_type, sym_other;
    ELFIO::Elf_Half section_index;
    CHECK(rel_text_ac.get_entries_num() == 3);
    // Entry 0
    rel_text_ac.get_entry(0, rel_offset, rel_symbol, rel_type, unused);
    CHECK(rel_offset == 6);
    CHECK(symtab_ac.get_symbol((ELFIO::Elf_Xword)rel_symbol, sym_name, sym_value, sym_size, sym_bind, sym_type,
                               section_index, sym_other));
    CHECK(sym_name == "d");
    // Entry 1
    rel_text_ac.get_entry(1, rel_offset, rel_symbol, rel_type, unused);
    CHECK(rel_offset == 2);
    CHECK(symtab_ac.get_symbol((ELFIO::Elf_Xword)rel_symbol, sym_name, sym_value, sym_size, sym_bind, sym_type,
                               section_index, sym_other));
    CHECK(sym_name == "i");
    // Entry 2
    rel_text_ac.get_entry(2, rel_offset, rel_symbol, rel_type, unused);
    CHECK(rel_offset == 5);
    CHECK(symtab_ac.get_symbol((ELFIO::Elf_Xword)rel_symbol, sym_name, sym_value, sym_size, sym_bind, sym_type,
                               section_index, sym_other));
    CHECK(sym_name == "i");
    CHECK(rel_data_ac.get_entries_num() == 1);
    // Entry 0
    rel_data_ac.get_entry(0, rel_offset, rel_symbol, rel_type, unused);
    CHECK(rel_offset == 3);
    CHECK(symtab_ac.get_symbol((ELFIO::Elf_Xword)rel_symbol, sym_name, sym_value, sym_size, sym_bind, sym_type,
                               section_index, sym_other));
    CHECK(sym_name == "d");
  }
}
