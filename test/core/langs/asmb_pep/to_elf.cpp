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
#include <set>
#include <sstream>
#include "core/compile/ir_linear/line_dot.hpp"
#include "core/compile/ir_linear/line_empty.hpp"
#include "core/langs/asmb/diagnostic_table.hpp"
#include "core/langs/asmb_pep/codegen.hpp"
#include "core/langs/asmb_pep/ir_visitor.hpp"
#include "core/langs/asmb_pep/parser.hpp"

namespace {
// Read what would actually be written, rather than the in-memory model of it.
ELFIO::elfio read_back(pepp::tc::ElfResult &result) {
  const auto bytes = pepp::tc::elf_bytes(result);
  std::istringstream in(std::string(reinterpret_cast<const char *>(bytes.data()), bytes.size()));
  ELFIO::elfio elf;
  REQUIRE(elf.load(in));
  return elf;
}
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

    REQUIRE(sections.size() == 3);
    auto elf = read_back(elf_result);
    // Every section carries its own address; there are no segments to take it from.
    for (const auto &[desc, _] : sections) {
      INFO(desc.name);
      const auto *sec = elf.sections[desc.name];
      REQUIRE(sec != nullptr);
      CHECK(sec->get_address() == desc.low_address);
    }
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

    // The blank first line joins .data instead of forcing an empty .text into existence.
    REQUIRE(sections.size() == 1);
    CHECK(sections[0].first.name == ".data");
    auto elf = read_back(elf_result);
    CHECK(elf.sections[".data"] != nullptr);
    CHECK(elf.sections[".text"] == nullptr);
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

    REQUIRE(sections.size() == 3);
    auto elf = read_back(elf_result);
    const auto *scratch = elf.sections["scratch"];
    REQUIRE(scratch != nullptr);
    CHECK(scratch->get_size() == 0);
    const auto *data = elf.sections[".data"];
    REQUIRE(data != nullptr);
    // Skipping empty sections used to shift every later index; the index baked into val must still be .data's.
    CHECK(symbol_tab->get("val").value()->section_index == data->get_index());
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
    // TODO: check these through .rel.text/.rel.data once the symbol table is written again. Until then, check the
    // relocations codegen records, which is what those sections were built from.
    REQUIRE(object_code.relocations.size() == 4);
    std::map<std::string, std::set<Result>> by_section;
    for (const auto &[entry, rel] : object_code.relocations)
      by_section[sections.at(rel.section_idx).first.name].insert(Result{(i16)rel.section_offset, std::string(entry->name)});
    CHECK(by_section[".text"] == std::set<Result>{{2, "i"}, {5, "i"}, {6, "d"}});
    CHECK(by_section[".data"] == std::set<Result>{{3, "d"}});
  }
}
