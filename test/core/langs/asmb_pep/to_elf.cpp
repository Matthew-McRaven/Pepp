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
#include "core/compile/ir_linear/line_dot.hpp"
#include "core/compile/ir_linear/line_empty.hpp"
#include "core/langs/asmb/diagnostic_table.hpp"
#include "core/langs/asmb_pep/codegen.hpp"
#include "core/langs/asmb_pep/ir_visitor.hpp"
#include "core/langs/asmb_pep/parser.hpp"

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
struct Result {
  i16 offset;
  std::string name;
  auto operator<=>(const Result &other) const {
    if (offset != other.offset) return offset <=> other.offset;
    else return name <=> other.name;
  };
  bool operator==(const Result &other) const { return (offset == other.offset) && (name == other.name); }
};
} // namespace

TEST_CASE("Pepp ASM codegen elf", "[scope:core][scope:core.langs][level:asmb3][level:asmb5][kind:unit][arch:*]") {
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
    auto code = pepp::tc::parser::flatten_macros(results);
    auto result = pepp::tc::pepp_split_to_sections(diag, code);
    CHECK(diag.count() == 0);

    auto symbol_tab = p.symbol_table();
    auto &sections = result.grouped_ir;
    auto addresses = pepp::tc::pepp_assign_addresses(sections);
    auto object_code = pepp::tc::pepp_to_object_code(addresses, sections);
    auto elf_result = pepp::tc::pepp_to_elf(sections, addresses, object_code, result.mmios);
    pepp::tc::write_symbol_table(elf_result, *symbol_tab, object_code);

    CHECK(sections.size() == 3);
    elf_result.elf->save("dummy.elf");
  }
  SECTION("A leading .SECTION does not create an empty implicit section") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(R"(
      .SECTION ".data","rwx"
      test:BR 10,i)"),
                    std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    auto code = pepp::tc::parser::flatten_macros(results);
    auto result = pepp::tc::pepp_split_to_sections(diag, code);
    CHECK(diag.count() == 0);
    auto symbol_tab = p.symbol_table();
    auto &sections = result.grouped_ir;
    auto addresses = pepp::tc::pepp_assign_addresses(sections);
    auto object_code = pepp::tc::pepp_to_object_code(addresses, sections);
    auto elf_result = pepp::tc::pepp_to_elf(sections, addresses, object_code, result.mmios);
    pepp::tc::write_symbol_table(elf_result, *symbol_tab, object_code);

    // The blank first line joins .data instead of forcing an empty .text into existence.
    REQUIRE(sections.size() == 1);
    CHECK(sections[0].first.name == ".data");
    CHECK(elf_result.elf->sections[".data"] != nullptr);
    CHECK(elf_result.elf->sections[".text"] == nullptr);
  }
  SECTION("An explicitly empty section is still emitted") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(R"(
      main:LDWA 5,i
      .SECTION "scratch","rw"
      .SECTION ".data","rw"
      val:.BLOCK 2)"),
                    std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    auto code = pepp::tc::parser::flatten_macros(results);
    auto result = pepp::tc::pepp_split_to_sections(diag, code);
    CHECK(diag.count() == 0);
    auto symbol_tab = p.symbol_table();
    auto &sections = result.grouped_ir;
    auto addresses = pepp::tc::pepp_assign_addresses(sections);
    auto object_code = pepp::tc::pepp_to_object_code(addresses, sections);
    auto elf_result = pepp::tc::pepp_to_elf(sections, addresses, object_code, result.mmios);
    pepp::tc::write_symbol_table(elf_result, *symbol_tab, object_code);

    REQUIRE(sections.size() == 3);
    auto *elf = elf_result.elf.get();
    const auto *scratch = elf->sections["scratch"];
    REQUIRE(scratch != nullptr);
    CHECK(scratch->get_size() == 0);
    const auto *data = elf->sections[".data"];
    REQUIRE(data != nullptr);

    // Skipping empty sections used to shift every later index; val must still point at .data.
    ELFIO::symbol_section_accessor symbols(*elf, elf->sections[".symtab"]);
    bool found = false;
    for (ELFIO::Elf_Xword it = 0; it < symbols.get_symbols_num(); ++it) {
      std::string name;
      ELFIO::Elf64_Addr value;
      ELFIO::Elf_Xword size;
      unsigned char bind, type, other;
      ELFIO::Elf_Half shndx;
      symbols.get_symbol(it, name, value, size, bind, type, shndx, other);
      if (name != "val") continue;
      found = true;
      CHECK(shndx == data->get_index());
    }
    CHECK(found);
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
)"),
                    std::make_shared<MR>());
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    for (auto &d : diag) std::cerr << d.second << "\n";
    auto code = pepp::tc::parser::flatten_macros(results);
    auto result = pepp::tc::pepp_split_to_sections(diag, code);
    CHECK(diag.count() == 0);

    auto symbol_tab = p.symbol_table();
    auto &sections = result.grouped_ir;
    auto addresses = pepp::tc::pepp_assign_addresses(sections);
    auto object_code = pepp::tc::pepp_to_object_code(addresses, sections);
    auto elf_result = pepp::tc::pepp_to_elf(sections, addresses, object_code, result.mmios);
    pepp::tc::write_symbol_table(elf_result, *symbol_tab, object_code);
    CHECK(object_code.relocations.size() == 4);
    elf_result.elf->save("needs_rel.elf");
    auto elf = elf_result.elf.get();
    REQUIRE(elf->sections.size() == 8);
    // By name rather than by index so that adding or removing a section elsewhere does not move these.
    auto symtab = elf->sections[".symtab"];
    REQUIRE(symtab != nullptr);
    auto symtab_ac = ELFIO::symbol_section_accessor(*elf, symtab);
    auto rel_text = elf->sections[".rel.text"];
    REQUIRE(rel_text != nullptr);
    auto rel_data = elf->sections[".rel.data"];
    REQUIRE(rel_data != nullptr);
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

    // Avoid depending on order of entries in std::multimap.
    std::set<Result> expected{
        {2, "i"},
        {5, "i"},
        {6, "d"},
    };
    for (size_t i = 0; i < expected.size(); i++) {
      rel_text_ac.get_entry(i, rel_offset, rel_symbol, rel_type, unused);
      CHECK(symtab_ac.get_symbol((ELFIO::Elf_Xword)rel_symbol, sym_name, sym_value, sym_size, sym_bind, sym_type,
                                 section_index, sym_other));
      Result local{(i16)rel_offset, sym_name};
      CHECK(expected.contains(local));
    }

    CHECK(rel_data_ac.get_entries_num() == 1);
    // Entry 0
    rel_data_ac.get_entry(0, rel_offset, rel_symbol, rel_type, unused);
    CHECK(rel_offset == 3);
    CHECK(symtab_ac.get_symbol((ELFIO::Elf_Xword)rel_symbol, sym_name, sym_value, sym_size, sym_bind, sym_type,
                               section_index, sym_other));
    CHECK(sym_name == "d");
  }
}
