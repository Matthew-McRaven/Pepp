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
#include <array>
#include <catch.hpp>
#include <elfio/elfio.hpp>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include "core/compile/ir_linear/line_dot.hpp"
#include "core/compile/ir_linear/line_empty.hpp"
#include "core/formats/elf/enums.hpp"
#include "core/langs/asmb/debug_line.hpp"
#include "core/langs/asmb/diagnostic_table.hpp"
#include "core/langs/asmb/listing.hpp"
#include "core/langs/asmb_driver.hpp"
#include "core/langs/asmb_pep/codegen.hpp"
#include "core/langs/asmb_pep/ir_visitor.hpp"
#include "core/langs/asmb_pep/parser.hpp"
#include "core/langs/asmb_pep/text_format.hpp"
#include "core/math/bitmanip/enums.hpp"

namespace {
// Numbering a listing needs no source text.
std::string no_text(const pepp::tc::LinearIR &) { return {}; }

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
  ELFIO::Elf64_Addr value;
};

std::vector<Symbol> symbols_of(ELFIO::elfio &elf) {
  auto *symtab = elf.sections[".symtab"];
  REQUIRE(symtab != nullptr);
  ELFIO::symbol_section_accessor accessor(elf, symtab);
  std::vector<Symbol> ret;
  for (ELFIO::Elf_Xword it = 0; it < accessor.get_symbols_num(); ++it) {
    Symbol got;
    ELFIO::Elf_Xword size;
    unsigned char other;
    REQUIRE(accessor.get_symbol(it, got.name, got.value, size, got.bind, got.type, got.shndx, other));
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
  oc.object_code = std::make_shared<pepp::bts::BlockStorage>();
  oc.object_code->allocate(total, 0xAA);
  oc.relocations.resize(specs.size());
  u32 at = 0;
  for (u16 i = 0; i < specs.size(); ++i) {
    const auto &spec = specs[i];
    SectionDescriptor desc{
        .name = spec.name, .flags = SectionFlags(spec.r, spec.w, spec.x, spec.z), .alignment = spec.align};
    desc.low_address = spec.low, desc.high_address = spec.low + spec.size + spec.overstate;
    desc.section_index = SectionDescriptor::section_base_index + i;
    prog.emplace_back(desc, IRProgram{});
    const u32 size = spec.z ? 0 : spec.size;
    oc.section_slices.emplace_back(oc.object_code, at, size);
    at += size;
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
struct Rela {
  ELFIO::Elf64_Addr offset;
  std::string symbol;
  unsigned type;
  ELFIO::Elf_Sxword addend;
  bool operator==(const Rela &) const = default;
};

std::vector<Rela> relocations_of(ELFIO::elfio &elf, const std::string &name, const std::vector<Symbol> &symbols) {
  auto *section = elf.sections[name];
  REQUIRE(section != nullptr);
  ELFIO::relocation_section_accessor accessor(elf, section);
  std::vector<Rela> ret;
  for (ELFIO::Elf_Xword it = 0; it < accessor.get_entries_num(); ++it) {
    Rela rel;
    ELFIO::Elf_Word symbol;
    REQUIRE(accessor.get_entry(it, rel.offset, symbol, rel.type, rel.addend));
    const auto &sym = symbols.at(symbol);
    // Like readelf, name a section symbol after its section.
    rel.symbol = sym.type == ELFIO::STT_SECTION ? elf.sections[sym.shndx]->get_name() : sym.name;
    ret.push_back(std::move(rel));
  }
  return ret;
}

// Every stage of assembling a Pep/10 `source`, for tests which inspect more than the ELF.
struct Assembled {
  std::shared_ptr<pepp::core::symbol::LeafTable> symbols;
  pepp::tc::IRProgram code; // Flattened, in source order.
  pepp::tc::PeppSectionAnalysisResults split;
  pepp::tc::IRMemoryAddressTable<pepp::tc::PeppAddress> addresses;
  pepp::tc::ProgramObjectCodeResult object_code;
  pepp::tc::ListingLayout layout; // Rows have no source text.
  pepp::tc::ElfResult elf;
};
struct Options {
  bool listing = false;     // Write the listing line program.
  bool relocatable = false; // Write ET_REL, so the debug sections carry relocations.
};

Assembled assemble(const char *source, Options options = {}) {
  using namespace pepp::tc;
  using namespace pepp::bts;
  DiagnosticTable diag;
  auto p = parser::PepParser(support::SeekableData{source}, std::make_shared<MacroRegistry>());
  Assembled ret{.symbols = p.symbol_table(), .code = parser::flatten_macros(p.parse(diag))};
  ret.split = pepp_split_to_sections(diag, ret.code);
  ret.addresses = pepp_assign_addresses(diag, ret.split.grouped_ir);
  ret.object_code = pepp_to_object_code(diag, ret.addresses, ret.split.grouped_ir);
  std::string messages;
  for (const auto &[_, diagnostic] : diag) messages += diagnostic.message + "\n";
  INFO(messages);
  REQUIRE(diag.count() == 0);

  ret.layout = layout_listing(ret.code, ret.addresses, pep_listing_bytes_per_line, no_text);
  const auto *listing = options.listing ? &ret.layout.rows : nullptr;
  auto &sections = ret.split.grouped_ir;
  if (!options.relocatable) {
    ret.elf = pepp_to_elf(sections, ret.addresses, ret.object_code, *ret.symbols, ret.split.mmios, {}, {},
                          ElfMachineType::EM_PEP10, listing);
    return ret;
  }
  // pepp_to_elf writes ET_EXEC, and write_debug_line only adds relocations to an ET_REL.
  ret.elf = sections_to_elf(ElfBits::b32, ElfEndian::be, ElfMachineType::EM_PEP10, sections, ret.object_code,
                            *ret.symbols);
  std::get<std::unique_ptr<PackedGrowableElfBE32>>(ret.elf.elf)->header.e_type =
      bits::to_underlying(ElfFileType::ET_REL);
  write_debug_line(ret.elf.elf, 2, bits::to_underlying(RelocationsPep::R_PEP10_ABS16),
                   bits::to_underlying(RelocationsPep::R_PEP10_ABS32), sections, ret.addresses, listing);
  return ret;
}
} // namespace

TEST_CASE("Pepp ASM codegen elf", "[scope:core][scope:core.langs][level:asmb3][level:asmb5][kind:unit][arch:*]") {
  using Lexer = pepp::tc::lex::PepLexer;
  using Parser = pepp::tc::parser::PepParser;
  using SymbolTable = pepp::core::symbol::LeafTable;
  using MR = pepp::tc::MacroRegistry;
  using namespace pepp::tc;
  SECTION("No ORG") {
    auto a = assemble(ex1, {.listing = true});
    REQUIRE(a.code.size() == 11);
    CHECK(std::dynamic_pointer_cast<EmptyLine>(a.code[0]));
    CHECK(std::dynamic_pointer_cast<DotSection>(a.code[1]));
    CHECK(std::dynamic_pointer_cast<DotSection>(a.code[3]));
    CHECK(std::dynamic_pointer_cast<DotSection>(a.code[6]));
    CHECK(std::dynamic_pointer_cast<DotSection>(a.code[8]));
    const auto &sections = a.split.grouped_ir;
    REQUIRE(sections.size() == 3);
    auto elf = read_back(a.elf);
    std::ostringstream streamed;
    pepp::tc::write_elf(a.elf, streamed);
    const auto bytes = pepp::tc::elf_bytes(a.elf);
    CHECK(streamed.str() == std::string(reinterpret_cast<const char *>(bytes.data()), bytes.size()));
    // .text is rwx; .data and memvec are both rw and contiguous, so they share one segment.
    CHECK(elf.segments.size() == 2);
    for (const auto &[desc, _] : sections) {
      INFO(desc.name);
      const auto *sec = elf.sections[desc.name];
      REQUIRE(sec != nullptr);
      CHECK(sec->get_address() == desc.low_address);
    }

    // The two line number sections each hold one DWARF 5 unit, mapping addresses back to 1-indexed lines.
    const auto bytes_of = [&](const char *name) {
      const auto *sec = elf.sections[name];
      REQUIRE(sec != nullptr);
      CHECK(sec->get_type() == ELFIO::SHT_PROGBITS);
      return bits::span<const u8>(reinterpret_cast<const u8 *>(sec->get_data()), static_cast<size_t>(sec->get_size()));
    };
    const auto encoded = bytes_of(pepp::tc::debugLineSection);
    const auto source_unit = pepp::bts::parse_line_program_unit<pepp::bts::ElfEndian::be>(encoded, 0);
    REQUIRE(source_unit.has_value());
    CHECK(source_unit->header.address_size == 2);
    CHECK(source_unit->header.files[0].path == pepp::tc::debugLineSourceFile);
    CHECK(source_unit->next_unit == encoded.size());
    const auto listing_encoded = bytes_of(pepp::tc::debugListingLineSection);
    const auto listing_unit = pepp::bts::parse_line_program_unit<pepp::bts::ElfEndian::be>(listing_encoded, 0);
    REQUIRE(listing_unit.has_value());
    CHECK(listing_unit->header.files[0].path == pepp::tc::debugLineListingFile);
    CHECK(listing_unit->next_unit == listing_encoded.size());
    const auto input = pepp::tc::to_input_elf(a.elf);
    const auto mappings = std::visit([](const auto &file) { return pepp::tc::read_line_mappings(*file); }, input);
    REQUIRE(mappings.has_value());
    const auto abs = bits::to_underlying(pepp::bts::SectionIndices::SHN_ABS);
    CHECK(std::ranges::all_of(*mappings, [&](const auto &mapping) { return mapping.section == abs; }));
    const auto address_of = [&](u32 source_line) -> std::optional<u32> {
      for (const auto &mapping : *mappings)
        if (mapping.source_line == source_line) return static_cast<u32>(mapping.address);
      return std::nullopt;
    };
    const u32 text = elf.sections[".text"]->get_address();
    CHECK(address_of(3) == text);     // bye:LDWA 10,d
    CHECK(address_of(8) == text + 3); // cruel:BR 0, after bye in the same .text
    CHECK(address_of(10) == elf.sections["memvec"]->get_address()); // World:.BYTE 0
    // Executables keep absolute addresses.
    CHECK(elf.sections[".rela.debug_line"] == nullptr);
    CHECK(elf.sections[std::string(".rela") + pepp::tc::debugListingLineSection] == nullptr);
  }
  SECTION("ET_REL .debug_line relocates one sequence per section") {
    const auto txt = R"(
.SECTION ".text","rx"
LDWA 1,i
LDWA 2,i
.SECTION ".data","rw"
.WORD 5
)";
    auto a = assemble(txt, {.listing = true, .relocatable = true});
    REQUIRE(a.split.grouped_ir.size() == 2);
    using namespace pepp::bts;
    const auto abs16 = bits::to_underlying(RelocationsPep::R_PEP10_ABS16);
    const auto abs32 = bits::to_underlying(RelocationsPep::R_PEP10_ABS32);
    auto elf = read_back(a.elf);
    const auto symbols = symbols_of(elf);
    // The source and listing units each have one sequence per section, and each DW_LNE_set_address is relocated.
    for (const auto *name : {debugLineSection, debugListingLineSection}) {
      INFO(name);
      const auto *lines = elf.sections[name];
      REQUIRE(lines != nullptr);
      const auto rela = std::string(".rela") + name;
      REQUIRE(elf.sections[rela] != nullptr);
      CHECK(elf.sections[rela]->get_info() == lines->get_index());
      const auto relocations = relocations_of(elf, rela, symbols);
      const std::array<std::string, 2> targets{".text", ".data"};
      REQUIRE(relocations.size() == targets.size());
      for (size_t it = 0; it < targets.size(); ++it) {
        CHECK(relocations[it] == Rela{relocations[it].offset, targets[it], abs16, 0});
        // S + A is the address already written into the operand.
        const auto *operand = reinterpret_cast<const u8 *>(lines->get_data()) + relocations[it].offset;
        CHECK(elf.sections[targets[it]]->get_address() == (u32(operand[0]) << 8 | operand[1]));
      }
    }
    // One range per section, each a DW_RLE_start_length after the 12-byte table header.
    CHECK(relocations_of(elf, ".rela.debug_rnglists", symbols) ==
          std::vector<Rela>{{13, ".text", abs16, 0}, {17, ".data", abs16, 0}});
    // The unit header's abbreviation offset, then DW_AT_stmt_list, DW_AT_ranges, and DW_AT_macros.
    CHECK(relocations_of(elf, ".rela.debug_info", symbols) ==
          std::vector<Rela>{{8, debugAbbrevSection, abs32, 0},
                            {13, debugLineSection, abs32, 0},
                            {17, debugRnglistsSection, abs32, sizeof(PackedRangeListHeader<ElfEndian::be>)},
                            {21, debugMacroSection, abs32, 0}});
    // The macro unit's debug_line_offset follows its version and flags.
    CHECK(relocations_of(elf, ".rela.debug_macro", symbols) == std::vector<Rela>{{3, debugLineSection, abs32, 0}});

    // Reading the relocations back tags each row with its section.
    const auto input = to_input_elf(a.elf);
    const auto mappings = std::visit([](const auto &file) { return read_line_mappings(*file); }, input);
    REQUIRE(mappings.has_value());
    std::set<u16> tagged;
    for (const auto &mapping : *mappings) tagged.insert(mapping.section);
    CHECK(tagged == std::set<u16>{static_cast<u16>(elf.sections[".text"]->get_index()),
                                  static_cast<u16>(elf.sections[".data"]->get_index())});
  }
  SECTION("Generating line number programs for ET_REL") {
    const auto txt = R"(.SECTION ".text","rx"
LDWA 1,i
x:
LDWA 2,i
.SECTION ".data","rw"
.WORD 5
)";
    auto a = assemble(txt, {.listing = true, .relocatable = true});
    REQUIRE(a.split.grouped_ir.size() == 2);
    auto elf = read_back(a.elf);
    const auto text = static_cast<u16>(elf.sections[".text"]->get_index());
    const auto data_section = static_cast<u16>(elf.sections[".data"]->get_index());

    const auto input = to_input_elf(a.elf);
    const auto rows = std::visit([](const auto &file) { return read_listing_rows(*file); }, input);
    REQUIRE(rows.has_value());
    // .SECTION has no address, so no row. x: has an address, but it is 0-sized.
    CHECK(*rows == std::vector<ListingRow>{{2, text, 0x0, 0x3},
                                           {3, text, 0x3, 0x3},
                                           {4, text, 0x3, 0x6},
                                           {6, data_section, 0x6, 0x8}});
  }
  SECTION("A leading .SECTION does not create an empty implicit section") {
    auto a = assemble(R"(
      .SECTION ".data","rwx"
      test:BR 10,i)");
    const auto &sections = a.split.grouped_ir;
    // The blank first line joins .data instead of forcing an empty .text into existence.
    REQUIRE(sections.size() == 1);
    CHECK(sections[0].first.name == ".data");
    auto elf = read_back(a.elf);
    CHECK(elf.sections[".data"] != nullptr);
    CHECK(elf.sections[".text"] == nullptr);
  }
  SECTION("Generate NOTE sections for MMIOs") {
    auto a = assemble(R"(
      .INPUT charIn
      .OUTPUT charOut
      charIn: .BLOCK 1
      charOut: .BLOCK 1)");
    auto elf = read_back(a.elf);
    REQUIRE(elf.sections[mmioNoteSection] != nullptr);
    CHECK(elf.sections[mmioNoteSection]->get_type() == ELFIO::SHT_NOTE);
    const auto input = to_input_elf(a.elf);
    const auto ios = std::visit([](const auto &file) { return MMIONoteReader(*file).declarations(); }, input);
    REQUIRE(ios.size() == 2);
    CHECK(ios[0].name == "charIn");
    CHECK(ios[0].type == IO::Type::kInput);
    CHECK(ios[0].minOffset == 0);
    CHECK(ios[1].name == "charOut");
    CHECK(ios[1].type == IO::Type::kOutput);
    CHECK(ios[1].minOffset == 1);
  }
  SECTION("An explicitly empty section is still emitted") {
    auto a = assemble(R"(
      main:LDWA 5,i
      .SECTION "scratch","rw"
      .SECTION ".data","rw"
      val:.BLOCK 2)");
    const auto &sections = a.split.grouped_ir;
    REQUIRE(sections.size() == 3);
    auto elf = read_back(a.elf);
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
  SECTION("relocations with undefined symbols") {
    auto a = assemble(R"(
			a:.BLOCK 2
			.EXPORT a
			LDWA i,i
      .BYTE i
      .WORD d
		  .SECTION ".data","rwx"
			LDWA a,i
			LDWA d,d
)");
    const auto &sections = a.split.grouped_ir;
    auto elf = read_back(a.elf);
    const auto symbols = symbols_of(elf);
    // The null symbol and a section symbol per section, then per reserved DWARF section, are local. a is global via
    // .EXPORT, while d and i are global because they are undefined.
    const auto named = 1 + sections.size() + debugSections.size();
    REQUIRE(symbols.size() == named + 3);
    CHECK(symbols[0].name.empty());
    // Every section gets one, in order, so prog section i is symbol [1 + i]
    for (u32 it = 0; it < sections.size(); ++it) {
      INFO(sections[it].first.name);
      const auto &sym = symbols.at(1 + it);
      CHECK(sym.name.empty());
      CHECK(sym.type == ELFIO::STT_SECTION);
      CHECK(sym.bind == ELFIO::STB_LOCAL);
      CHECK(sym.shndx == elf.sections[sections[it].first.name]->get_index());
    }
    for (size_t it = 0; it < debugSections.size(); ++it) {
      INFO(debugSections[it]);
      CHECK(symbols.at(1 + sections.size() + it).type == ELFIO::STT_SECTION);
      CHECK(symbols.at(1 + sections.size() + it).shndx == elf.sections[debugSections[it]]->get_index());
    }
    CHECK(symbols[named].name == "a");
    CHECK(symbols[named].bind == ELFIO::STB_GLOBAL);
    CHECK(symbols[named].shndx == elf.sections[".text"]->get_index());
    CHECK(symbols[named + 1].name == "d");
    CHECK(symbols[named + 1].bind == ELFIO::STB_GLOBAL);
    CHECK(symbols[named + 1].shndx == ELFIO::SHN_UNDEF);
    CHECK(symbols[named + 2].name == "i");
    CHECK(symbols[named + 2].bind == ELFIO::STB_GLOBAL);
    CHECK(symbols[named + 2].shndx == ELFIO::SHN_UNDEF);
    CHECK(elf.sections[".symtab"]->get_info() == named); // One past the last local.

    using enum pepp::bts::RelocationsPep;
    // Offsets are of the patched operand specifier, not instruction specifier
    CHECK(relocations_of(elf, ".rela.text", symbols) ==
          std::vector<Rela>{{3, "i", bits::to_underlying(R_PEP10_ABS16), 0},
                            {5, "i", bits::to_underlying(R_PEP10_ABS8), 0},
                            {6, "d", bits::to_underlying(R_PEP10_ABS16), 0}});
    // a is global, so it is relocated against itself rather than as an offset into .text.
    CHECK(relocations_of(elf, ".rela.data", symbols) ==
          std::vector<Rela>{{1, "a", bits::to_underlying(R_PEP10_ABS16), 0},
                            {4, "d", bits::to_underlying(R_PEP10_ABS16), 0}});
    for (const auto *name : {".rela.text", ".rela.data"}) {
      INFO(name);
      const auto *rela = elf.sections[name];
      CHECK(rela->get_type() == ELFIO::SHT_RELA);
      CHECK(rela->get_link() == elf.sections[".symtab"]->get_index());
    }
    CHECK(elf.sections[".rela.text"]->get_info() == elf.sections[".text"]->get_index());
    CHECK(elf.sections[".rela.data"]->get_info() == elf.sections[".data"]->get_index());
  }
  SECTION("relocations with defined symbols") {
    auto a = assemble(R"(
			c:.EQUATE 5
			.BLOCK 1
			l:.WORD 0
			LDWA l,d
			LDWA c,i
			LDWA m,d
		  .SECTION ".data","rw"
			.BLOCK 2
			m:.WORD 0
			.WORD l
			.BYTE l
			.WORD c
			.WORD m
)");
    auto elf = read_back(a.elf);
    const auto symbols = symbols_of(elf);
    // Section symbols hold their section's address
    for (const auto &sym : symbols) {
      if (sym.type != ELFIO::STT_SECTION) continue;
      INFO(elf.sections[sym.shndx]->get_name());
      CHECK(sym.value == elf.sections[sym.shndx]->get_address());
    }

    using enum pepp::bts::RelocationsPep;
    // Locals are offsets from their section's symbol. The constant c never moves.
    CHECK(relocations_of(elf, ".rela.text", symbols) ==
          std::vector<Rela>{{4, ".text", bits::to_underlying(R_PEP10_ABS16), 1},
                            {10, ".data", bits::to_underlying(R_PEP10_ABS16), 2}});
    CHECK(relocations_of(elf, ".rela.data", symbols) ==
          std::vector<Rela>{{4, ".text", bits::to_underlying(R_PEP10_ABS16), 1},
                            {6, ".text", bits::to_underlying(R_PEP10_ABS8), 1},
                            {9, ".data", bits::to_underlying(R_PEP10_ABS16), 2}});
  }
  SECTION("relocations with symbol + constant") {
    auto a = assemble(R"(
			c:.EQUATE 5
			.BLOCK 1
			l:.WORD 0
			LDWA l+3,d
			LDWA 2 + u,d
			LDWA c+1,i
			LDWA u + -2,d
			LDWA l - 1,d
			LDWA u - 2,d
			LDWA 1 + l - 4,d
)");
    auto elf = read_back(a.elf);
    const auto symbols = symbols_of(elf);

    using enum pepp::bts::RelocationsPep;
    // The constant is added to the addend ELF derives for the symbol: l is 1 past its section symbol. The constant c
    // never moves, so c+1 is not relocated; its sum is simply stored.
    CHECK(relocations_of(elf, ".rela.text", symbols) ==
          std::vector<Rela>{{4, ".text", bits::to_underlying(R_PEP10_ABS16), 1 + 3},
                            {7, "u", bits::to_underlying(R_PEP10_ABS16), 2},
                            {13, "u", bits::to_underlying(R_PEP10_ABS16), -2},
                            {16, ".text", bits::to_underlying(R_PEP10_ABS16), 1 - 1},
                            {19, "u", bits::to_underlying(R_PEP10_ABS16), -2},
                            {22, ".text", bits::to_underlying(R_PEP10_ABS16), 1 + 1 - 4}});
    const auto *text = elf.sections[".text"]->get_data();
    CHECK(static_cast<u8>(text[9]) == 0xC0); // LDWA c+1,i
    CHECK(static_cast<u8>(text[10]) == 0x00);
    CHECK(static_cast<u8>(text[11]) == 0x06);
  }
  SECTION("location counters") {
    // The second . aliases the label on its line.
    auto a = assemble("LDWA 0,i\nBR .\nl: BR . + 3");
    auto elf = read_back(a.elf);
    const auto symbols = symbols_of(elf);

    const auto *text = elf.sections[".text"]->get_data();
    CHECK(static_cast<u8>(text[4]) == 0x00); // BR .
    CHECK(static_cast<u8>(text[5]) == 0x03);
    CHECK(static_cast<u8>(text[7]) == 0x00); // BR . + 3
    CHECK(static_cast<u8>(text[8]) == 0x09);
    // . moves with its section, like a label.
    using enum pepp::bts::RelocationsPep;
    CHECK(relocations_of(elf, ".rela.text", symbols) ==
          std::vector<Rela>{{4, ".text", bits::to_underlying(R_PEP10_ABS16), 3},
                            {7, ".text", bits::to_underlying(R_PEP10_ABS16), 6 + 3}});
    // Location counters are serialized to ELF file
    for (const auto &symbol : symbols) CHECK(!symbol.name.starts_with("<."));
  }
  SECTION("operands referencing symbols must be symbol [+ constant]") {
    const auto diagnostics = [&](const char *source) {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data(source), std::make_shared<MR>());
      auto results = p.parse(diag);
      REQUIRE(diag.count() == 0);
      auto code = pepp::tc::parser::flatten_macros(results);
      (void)pepp::tc::pepp_split_to_sections(diag, code);
      return diag.count();
    };
    CHECK(diagnostics("a:.BLOCK 1\nLDWA a,d\nLDWA a + 1,d\nLDWA 1 + a,d\nLDWA 1 + 2,d") == 0);
    CHECK(diagnostics("a:.BLOCK 1\nLDWA a - 1,d\nm:.EQUATE 1 + 2") == 0);
    CHECK(diagnostics("a:.BLOCK 1\nb:.BLOCK 1\nLDWA a + b,d") == 1);
    CHECK(diagnostics("a:.BLOCK 1\nb:.BLOCK 1\nLDWA a - b,d") == 1);
    CHECK(diagnostics("a:.BLOCK 1\nLDWA 1 - a,d") == 1); // A - S is not a valid relocation type.
    // Constants, including equates, are gathered first, so this reduces to constant + symbol.
    CHECK(diagnostics("k:.EQUATE 4\na:.BLOCK 1\nLDWA 1 + a - k * 3,d") == 0);
    CHECK(diagnostics("a:.BLOCK 1\nLDWA (a + 1) * 2,d") == 1); // A scaled symbol cannot be relocated.
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

TEST_CASE("Listings work as expected when re-opening sections",
          "[scope:core][scope:core.langs][kind:unit][arch:pep10]") {
  const auto source = R"(         .SECTION "text", "rx"
         LDWA    1,i
         .SECTION "data", "rw"
x:       .WORD   5
         .SECTION "text", "rx"
         RET
)";
  auto result = pepp::tc::assemble_pep10(pepp::tc::Pep10DriverConfig{}, {.listing = true}, source);
  REQUIRE(result.ok());
  const auto listing = result.listing();
  REQUIRE(listing.has_value());
  const auto input = pepp::tc::to_input_elf(*result.elf());
  const auto mappings = std::visit([](const auto &file) { return pepp::tc::read_line_mappings(*file); }, input);
  REQUIRE(mappings.has_value());
  const auto stop = std::ranges::find(*mappings, 6u, &pepp::tc::LineMapping::source_line);
  REQUIRE(stop != mappings->end());
  REQUIRE(stop->listing_line != 0);
  REQUIRE(stop->listing_line <= listing->size());
  CHECK((*listing)[stop->listing_line - 1].find("RET") != std::string::npos);
}

TEST_CASE("Listing line program rows match printout", "[scope:core][scope:core.langs][kind:int][arch:pep9]") {
  using namespace pepp::tc;
  auto result = assemble_pep9(Pep9DriverConfig{}, {.listing = true}, R"(LDWA 1,i
.BLOCK 6
.BURN 0xFFFF
RET
)");
  REQUIRE(result.ok());
  const auto input = to_input_elf(*result.elf());
  const auto rows = std::visit([](const auto &file) { return read_listing_rows(*file); }, input);
  REQUIRE(rows.has_value());
  // Lines before .BURN keep their addresses but do not print object code, so they have no length or continuation rows.
  const auto abs = bits::to_underlying(pepp::bts::SectionIndices::SHN_ABS);
  CHECK(*rows ==
        std::vector<ListingRow>{{1, abs, 0xFFF6, 0xFFF6}, {2, abs, 0xFFF9, 0xFFF9}, {4, abs, 0xFFFF, 0x10000}});
  const auto listing = result.listing();
  REQUIRE(listing.has_value());
  REQUIRE(listing->size() >= 4);
  CHECK((*listing)[3].find("RET") != std::string::npos);
}

TEST_CASE("Listings are dervied from ELF", "[scope:core][scope:core.langs][kind:int][arch:pep10]") {
  using namespace pepp::tc;
  using namespace pepp::bts;
  auto a = assemble("LDWA 1,i\nRET\n", {.listing = true});
  auto &file = *std::get<std::unique_ptr<PackedGrowableElfBE32>>(a.elf.elf);
  const auto data_of = [&](const char *name) { return file.section_data[find_named_section(file, name)]; };
  const auto listing = [&] {
    auto ret = format_listing(to_input_elf(a.elf), a.layout.text, pep_listing_style);
    REQUIRE(ret.has_value());
    return *ret;
  };
  CHECK(listing() == std::vector<std::string>{"0000 C00001 ", "0003 01     "});

  SECTION("Object code comes from section data") {
    auto text = data_of(".text");
    text->get(0, text->size())[1] = 0xAB;
  }

  SECTION("Addresses come from the listing line program") {
    // Move .text and its listing rows together, as a linker would.
    auto &header = file.section_headers[find_named_section(file, ".text")];
    header.sh_addr = static_cast<u32>(header.sh_addr) + 0x100;
    auto lines = data_of(debugListingLineSection);
    const auto bytes = lines->get(0, lines->size());
    const auto unit = parse_line_program_unit<ElfEndian::be>(bytes, 0);
    REQUIRE(unit.has_value());
    const auto program_start = static_cast<size_t>(unit->program.data() - bytes.data());
    std::set<size_t> operands;
    const auto ran = run_line_program<ElfEndian::be>(*unit, [&](const DebugLineState &, std::optional<size_t> operand) {
      if (operand) operands.insert(program_start + *operand);
    });
    REQUIRE(ran.has_value());
    REQUIRE(!operands.empty());
    for (const auto offset : operands) {
      const u16 address = (bytes[offset] << 8 | bytes[offset + 1]) + 0x100;
      bytes[offset] = address >> 8, bytes[offset + 1] = address & 0xFF;
    }
  }
}
