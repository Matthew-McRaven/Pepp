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

#include <algorithm>
#include <catch.hpp>
#include <elfio/elfio.hpp>
#include <set>
#include <sstream>
#include <string>
#include <vector>
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
struct Symbol {
  std::string name;
  unsigned char bind, type;
  ELFIO::Elf_Half shndx;
};

std::vector<Symbol> symbols_of(ELFIO::elfio &elf) {
  auto *symtab = elf.sections[".symtab"];
  REQUIRE(symtab != nullptr);
  ELFIO::symbol_section_accessor accessor(elf, symtab);
  std::vector<Symbol> ret;
  for (ELFIO::Elf_Xword it = 0; it < accessor.get_symbols_num(); ++it) {
    Symbol got;
    ELFIO::Elf64_Addr value;
    ELFIO::Elf_Xword size;
    unsigned char other;
    REQUIRE(accessor.get_symbol(it, got.name, value, size, got.bind, got.type, got.shndx, other));
    ret.push_back(std::move(got));
  }
  return ret;
}

// A section with a known address and size, without going through the assembler's address assignment.
struct Spec {
  std::string name;
  bool r, w, x, z;
  u32 low, size;
  u16 align = 1;
  u32 overstate = 0; // How far the descriptor's address range runs past the data, as .ORG can leave it.
};

pepp::tc::ElfResult from_specs(const std::vector<Spec> &specs) {
  using namespace pepp::tc;
  std::vector<std::pair<SectionDescriptor, IRProgram>> prog;
  ProgramObjectCodeResult oc;
  u32 total = 0;
  for (const auto &spec : specs)
    if (!spec.z) total += spec.size;
  oc.object_code.assign(total, 0xAA);
  u32 at = 0;
  for (u16 i = 0; i < specs.size(); ++i) {
    const auto &spec = specs[i];
    SectionDescriptor desc{
        .name = spec.name, .flags = SectionFlags(spec.r, spec.w, spec.x, spec.z), .alignment = spec.align};
    desc.low_address = spec.low, desc.high_address = spec.low + spec.size + spec.overstate;
    desc.section_index = SectionDescriptor::section_base_index + i;
    prog.emplace_back(desc, IRProgram{});
    if (spec.z) oc.section_spans.push_back({});
    else oc.section_spans.push_back({std::span<u8>(oc.object_code.data() + at, spec.size)}), at += spec.size;
  }
  const pepp::core::symbol::LeafTable symbols(2);
  return sections_to_elf(pepp::bts::ElfBits::b32, pepp::bts::ElfEndian::be, pepp::bts::ElfMachineType::EM_PEP10,
                         prog, oc, symbols);
}

struct Segment {
  u32 flags;
  u64 vaddr, filesz, memsz, align, offset;
};

std::vector<Segment> segments_of(pepp::tc::ElfResult &result) {
  auto elf = read_back(result);
  std::vector<Segment> ret;
  for (const auto &seg : elf.segments) {
    REQUIRE(seg->get_type() == ELFIO::PT_LOAD);
    ret.push_back({seg->get_flags(), seg->get_virtual_address(), seg->get_file_size(), seg->get_memory_size(),
                   seg->get_align(), seg->get_offset()});
    const auto align = std::max<u64>(ret.back().align, 1);
    CHECK(ret.back().offset % align == ret.back().vaddr % align);
    CHECK(ret.back().memsz > 0);
  }
  return ret;
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
    auto elf_result = pepp::tc::pepp_to_elf(sections, addresses, object_code, *symbol_tab, result.mmios);

    REQUIRE(sections.size() == 3);
    auto elf = read_back(elf_result);
    // .text is rwx; .data and memvec are both rw and contiguous, so they share one segment.
    CHECK(elf.segments.size() == 2);
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
    auto elf_result = pepp::tc::pepp_to_elf(sections, addresses, object_code, *symbol_tab, result.mmios);

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
    auto elf_result = pepp::tc::pepp_to_elf(sections, addresses, object_code, *symbol_tab, result.mmios);

    REQUIRE(sections.size() == 3);
    auto elf = read_back(elf_result);
    const auto *scratch = elf.sections["scratch"];
    REQUIRE(scratch != nullptr);
    CHECK(scratch->get_size() == 0);
    const auto *data = elf.sections[".data"];
    REQUIRE(data != nullptr);
    // Skipping empty sections used to shift every later index; val must still point at .data.
    const auto symbols = symbols_of(elf);
    const auto val = std::find_if(symbols.begin(), symbols.end(), [](const auto &sym) { return sym.name == "val"; });
    REQUIRE(val != symbols.end());
    CHECK(val->shndx == data->get_index());
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
    auto elf_result = pepp::tc::pepp_to_elf(sections, addresses, object_code, *symbol_tab, result.mmios);
    auto elf = read_back(elf_result);
    const auto symbols = symbols_of(elf);
    // The null symbol, then locals (i and d are referenced but never defined), then a, which .EXPORT made global.
    REQUIRE(symbols.size() == 4);
    CHECK(symbols[0].name.empty());
    CHECK(symbols[1].name == "d");
    CHECK(symbols[1].shndx == ELFIO::SHN_UNDEF);
    CHECK(symbols[2].name == "i");
    CHECK(symbols[2].shndx == ELFIO::SHN_UNDEF);
    CHECK(symbols[3].name == "a");
    CHECK(symbols[3].bind == ELFIO::STB_GLOBAL);
    CHECK(symbols[3].shndx == elf.sections[".text"]->get_index());
    CHECK(elf.sections[".symtab"]->get_info() == 3); // One past the last local.

    // TODO: check these through .rel.text/.rel.data once relocations are written. Until then, check the relocations
    // codegen records, which is what those sections are built from.
    REQUIRE(object_code.relocations.size() == 4);
    std::map<std::string, std::set<Result>> by_section;
    for (const auto &[entry, rel] : object_code.relocations)
      by_section[sections.at(rel.section_idx).first.name].insert(
          Result{(i16)rel.section_offset, std::string(entry->name)});
    CHECK(by_section[".text"] == std::set<Result>{{2, "i"}, {5, "i"}, {6, "d"}});
    CHECK(by_section[".data"] == std::set<Result>{{3, "d"}});
  }
}

TEST_CASE("Pepp ASM segments derive from sections",
          "[scope:core][scope:core.langs][level:asmb3][level:asmb5][kind:unit][arch:*]") {
  constexpr u32 rx = 5, rw = 6; // PF_R | PF_X, PF_R | PF_W

  SECTION("Contiguous sections of same flags combine into one segment") {
    auto result = from_specs({{"a", true, false, true, false, 0, 3}, {"b", true, false, true, false, 3, 3}});
    const auto segs = segments_of(result);
    REQUIRE(segs.size() == 1);
    CHECK(segs[0].flags == rx);
    CHECK(segs[0].vaddr == 0);
    CHECK(segs[0].filesz == 6);
    CHECK(segs[0].memsz == 6);
  }

  SECTION("Sections without matching flags are not combined.") {
    auto result = from_specs({{"a", true, false, true, false, 0, 3}, {"b", true, true, false, false, 3, 2}});
    const auto segs = segments_of(result);
    REQUIRE(segs.size() == 2);
    CHECK(segs[0].flags == rx);
    CHECK(segs[1].flags == rw);
  }

  SECTION("A trailing NOBITS shares the segment with sections before it") {
    auto result = from_specs({{"data", true, true, false, false, 0, 2}, {"bss", true, true, false, true, 2, 8}});
    const auto segs = segments_of(result);
    REQUIRE(segs.size() == 1);
    // The loader zero-fills what p_filesz does not cover.
    CHECK(segs[0].filesz == 2);
    CHECK(segs[0].memsz == 10);
  }

  SECTION("File data after a NOBITS starts a new segment") {
    auto result = from_specs({{"data", true, true, false, false, 0, 2},
                              {"bss", true, true, false, true, 2, 4},
                              {"more", true, true, false, false, 6, 2}});
    const auto segs = segments_of(result);
    REQUIRE(segs.size() == 2);
    CHECK(segs[1].vaddr == 6);
  }

  SECTION("An address gap starts a new segment") {
    auto result = from_specs({{"lo", true, true, false, false, 0x10, 2}, {"hi", true, true, false, false, 0x100, 2}});
    const auto segs = segments_of(result);
    REQUIRE(segs.size() == 2);
    CHECK(segs[0].filesz == 2);
    CHECK(segs[1].filesz == 2);
    CHECK(segs[1].offset - segs[0].offset == 2);
  }

  SECTION("Alignment/padding does not break contiguity") {
    auto result = from_specs({{"a", true, false, true, false, 0, 3}, {"b", true, false, true, false, 4, 2, 4}});
    const auto segs = segments_of(result);
    REQUIRE(segs.size() == 1);
    // The strictest member sets the segment's alignment.
    CHECK(segs[0].align == 4);
    CHECK(segs[0].memsz == 6);
  }

  SECTION("An empty section don't start a segment") {
    auto result = from_specs({{"text", true, true, true, false, 0, 4},
                              {"empty", true, true, false, false, 4, 0},
                              {"data", true, true, false, false, 4, 2}});
    const auto segs = segments_of(result);
    REQUIRE(segs.size() == 2);
    CHECK(segs[1].vaddr == 4);
    CHECK(segs[1].memsz == 2);
  }

  SECTION("An empty section joins a segment it fits in") {
    auto result = from_specs({{"a", true, true, false, false, 0, 2},
                              {"empty", true, true, false, false, 2, 0},
                              {"b", true, true, false, false, 2, 2}});
    const auto segs = segments_of(result);
    REQUIRE(segs.size() == 1);
    CHECK(segs[0].memsz == 4);
  }
}
