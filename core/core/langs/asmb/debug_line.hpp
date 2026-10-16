#pragma once
#include <algorithm>
#include <array>
#include <expected>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include "core/formats/dwarf/info.hpp"
#include "core/compile/ir_linear/line_macro.hpp"
#include "core/formats/dwarf/line_program.hpp"
#include "core/formats/dwarf/macro.hpp"
#include "core/formats/dwarf/rnglists.hpp"
#include "core/formats/elf/enums_sections.hpp"
#include "core/formats/elf/packed_access_relocations.hpp"
#include "core/formats/elf/packed_access_symbol.hpp"
#include "core/formats/elf/packed_elf.hpp"
#include "core/formats/elf/packed_ops.hpp"
#include "core/integers.h"
#include "core/langs/asmb/codegen.hpp"
#include "core/langs/asmb/elf_symtab.hpp"
#include "core/langs/asmb/ir_program.hpp"

namespace pepp::tc {

static inline const auto debugAbbrevSection = ".debug_abbrev";
static inline const auto debugInfoSection = ".debug_info";
static inline const auto debugLineSection = ".debug_line";
static inline const auto debugLineStrSection = ".debug_line_str";
static inline const auto debugMacroSection = ".debug_macro";
static inline const auto debugRnglistsSection = ".debug_rnglists";
// Line program units mapping addresses to listing lines. The name is outside DWARF's .debug_ namespace, so DWARF
// consumers ignore it, while binutils should still treat it as debug information.
static inline const auto debugListingLineSection = ".debug.pepp.listing_line";
// The sections write_debug_line fills. sections_to_elf reserves them before .symtab, so each gets a section symbol for
// relocations to target.
static inline const std::array<const char *, 6> debugSections{
    debugAbbrevSection,   debugInfoSection,        debugLineSection,
    debugRnglistsSection, debugListingLineSection, debugMacroSection};
// The producer named by DW_AT_producer.
static inline const auto debugProducer = "Pepp";

// .debug_line holds a DWARF 5 line program unit mapping addresses to source lines. debugListingLineSection holds a unit
// in the same format mapping the same addresses to listing lines. Each unit names one file, which defaults to these.
static inline const auto debugLineSourceFile = "source";
static inline const auto debugLineListingFile = "listing";

// Numbers listing lines in source order, as a listing with `bytes_per_line` bytes of object code per row prints them.
// Lines hidden from the listing get no number.
IR2ListingLineMap number_listing_lines(const IRProgram &program, const ProgramObjectCodeResult &object_code,
                                       u32 bytes_per_line);

// One address's lines, merged from both units. Lines are 1-based, and 0 means the address has no such line.
// In a relocatable file, `address` is an offset into `section`. SHN_ABS means `address` is absolute.
struct LineMapping {
  u16 section = bits::to_underlying(pepp::bts::SectionIndices::SHN_ABS);
  u64 address = 0;
  u32 source_line = 0, listing_line = 0;
  bool operator==(const LineMapping &) const = default;
};

namespace detail {
pepp::bts::LineProgramHeader debug_line_header(u8 address_size, std::string_view directory, std::string_view file);

// Row numbers are 0-indexed, while line numbers are 1-indexed, so 0 means "no line number".
inline u32 source_line(const LinearIR &line) {
  return line.source_interval.valid() ? 1 + line.source_interval.lower().row : 0;
}
inline u32 listing_line(const IR2ListingLineMap &listing, const LinearIR &line) {
  const auto it = listing.find(&line);
  return it != listing.cend() ? 1 + it->second : 0;
}
// An address or offset at `offset` in a DWARF section, relocated against the STT_SECTION symbol of `section`.
struct SectionRelocation {
  size_t offset;
  u16 section;
  u64 addend;
};

// Writes .rela<name of target>, relocating each entry against its requested section's symbol.
template <pepp::bts::ElfBits B, pepp::bts::ElfEndian E>
void write_section_relocations(pepp::bts::PackedGrowableElfFile<B, E> &elf, u16 rel_target_shndx, std::string_view name,
                               u32 type, const std::vector<SectionRelocation> &relocations) {
  using namespace pepp::bts;
  if (relocations.empty()) return;
  u16 symtab = 0;
  for (u16 it = 1; it < elf.section_headers.size() && symtab == 0; ++it)
    if (elf.section_headers[it].sh_type == bits::to_underlying(SectionTypes::SHT_SYMTAB)) symtab = it;
  if (symtab == 0) throw std::logic_error("write_section_relocations: no .symtab to relocate against");
  // Collect the symbols before adding a section, since the reader holds references into the section headers.
  std::vector<u32> section_symbol(elf.section_headers.size(), 0);
  {
    PackedSymbolReader<B, E> symbols(elf, symtab);
    for (u32 it = 1; it < symbols.symbol_count(); ++it)
      if (const auto symbol = symbols.get_symbol(it); symbol.st_type() == SymbolType::STT_SECTION)
        section_symbol.at(symbol.st_shndx) = it;
  }
  const auto rela = add_named_rela(elf, ".rela" + std::string(name), symtab, rel_target_shndx);
  elf.section_headers[rela].sh_flags = bits::to_underlying(pepp::bts::SectionFlags::SHF_INFO_LINK);
  elf.section_headers[rela].sh_addralign = sizeof(word<B>);
  PackedRelocationWriter<B, E> writer(elf, rela);
  for (const auto &[offset, section, addend] : relocations) {
    const u32 symbol = section_symbol.at(section);
    if (symbol == 0) throw std::logic_error("write_section_relocations: section has no STT_SECTION symbol");
    writer.add_rela(offset, type, symbol, static_cast<sword<B>>(addend));
  }
}
// The compile unit's only abbreviation. Section 3.1.1 lists these attributes. The order matches that of GNU as, with
// the addition of DW_AT_macros.
inline constexpr std::array<pepp::bts::AttributeSpecification, 7> unit_attributes{{
    {pepp::bts::AttributeNames::DW_AT_stmt_list, pepp::bts::AttributeForms::DW_FORM_sec_offset},
    {pepp::bts::AttributeNames::DW_AT_ranges, pepp::bts::AttributeForms::DW_FORM_sec_offset},
    {pepp::bts::AttributeNames::DW_AT_macros, pepp::bts::AttributeForms::DW_FORM_sec_offset},
    {pepp::bts::AttributeNames::DW_AT_name, pepp::bts::AttributeForms::DW_FORM_string},
    {pepp::bts::AttributeNames::DW_AT_comp_dir, pepp::bts::AttributeForms::DW_FORM_string},
    {pepp::bts::AttributeNames::DW_AT_producer, pepp::bts::AttributeForms::DW_FORM_string},
    {pepp::bts::AttributeNames::DW_AT_language, pepp::bts::AttributeForms::DW_FORM_data2},
}};
inline constexpr std::array<pepp::bts::Abbreviation, 1> unit_abbreviations{{
    {1, pepp::bts::Tags::DW_TAG_compile_unit, pepp::bts::ChildrenDetermination::DW_CHILDREN_no, unit_attributes},
}};

// Convert a macro IR line into a well-formatted definition string per Section 6.3.2.2
std::string define_string(const InlineMacroDefinition &definition);
} // namespace detail

// Fills the sections of debugSections. Writes the source line program to .debug_line and the listing line program to a
// separate section. A compile unit is written so that address-to-line tools work as expected. Sections are emitted as
// independent sequences. For ET_REL files, the initial addresses are relocated against the STT_SECTION symbol of the
// section they came from. Advances assume the linker does not apply relaxations.
template <typename Address>
void write_debug_line(pepp::bts::AnyGrowableElf &elf, u8 address_size, u32 address_relocation, u32 offset_relocation,
                      const std::vector<std::pair<SectionDescriptor, IRProgram>> &prog,
                      const IRMemoryAddressTable<Address> &addrs, const IR2ListingLineMap &listing,
                      const DebugLineFiles &files = {},
                      const std::vector<std::shared_ptr<const InlineMacroDefinition>> &macros = {}) {
  const auto name_or = [](const std::optional<std::string> &name, const char *fallback) -> std::string_view {
    return name && !name->empty() ? std::string_view(*name) : std::string_view(fallback);
  };
  const auto source_name = name_or(files.source, debugLineSourceFile);
  const auto listing_name = name_or(files.listing, debugLineListingFile);
  const auto directory = name_or(files.directory, ".");
  auto f = [&](auto &file) {
    if (!file) return;
    using File = std::remove_cvref_t<decltype(*file)>;
    constexpr auto E = File::elf_endian;
    const auto section = [&](const char *name) {
      if (const auto index = pepp::bts::find_named_section(*file, name); index != 0) return index;
      throw std::logic_error(std::string("write_debug_line: sections_to_elf did not reserve ") + name);
    };
    const auto line_index = section(debugLineSection), abbrev_index = section(debugAbbrevSection);
    const auto info_index = section(debugInfoSection), rnglists_index = section(debugRnglistsSection);
    const auto listing_index = section(debugListingLineSection), macro_index = section(debugMacroSection);

    std::vector<detail::SectionRelocation> line_relocations, listing_relocations;
    const auto write_unit = [&](u16 target, std::vector<detail::SectionRelocation> &relocations, std::string_view name,
                                bool for_listing) {
      pepp::bts::LineProgramWriter<E> writer(*file->section_data[target],
                                             detail::debug_line_header(address_size, directory, name));
      for (const auto &[desc, lines] : prog) {
        const auto first_operand = writer.address_operands().size();
        for (const auto &line : lines) {
          const auto addr_it = addrs.find(line.get());
          if (addr_it == addrs.cend()) continue;
          const u32 number = for_listing ? detail::listing_line(listing, *line) : detail::source_line(*line);
          if (number != 0) writer.row(addr_it->second.address, addr_it->second.size, 1, number);
        }
        // A linker places each section on its own, so a sequence must not continue into the next one.
        writer.end_sequence();
        const auto &operands = writer.address_operands();
        for (auto it = first_operand; it < operands.size(); ++it)
          relocations.push_back({operands[it].offset, desc.section_index, operands[it].address - desc.low_address});
      }
      return writer.finish();
    };
    const auto source_unit = write_unit(line_index, line_relocations, source_name, false);
    write_unit(listing_index, listing_relocations, listing_name, true);

    // Extract the spanned ranges of the source program and emit to a range list.
    std::vector<detail::SectionRelocation> range_relocations;
    pepp::bts::RangeListWriter<E> ranges(*file->section_data[rnglists_index], address_size);
    const auto list = ranges.list_offset();
    for (const auto &[desc, lines] : prog) {
      // NOBITS has no data, so its extent comes from its addresses.
      const u64 size =
          desc.flags.z ? desc.high_address - desc.low_address : file->section_data[desc.section_index]->size();
      if (size == 0) continue;
      range_relocations.push_back({ranges.start_length(desc.low_address, size), desc.section_index, 0});
    }
    ranges.end_list();
    ranges.finish();

    // Refer to the source unit, so that DW_MACRO_start_file names the source rather than the listing.
    pepp::bts::MacroUnitWriter<E> macro_unit(*file->section_data[macro_index], static_cast<u32>(source_unit));
    // Section 6.3.3.1: the primary source file's DW_MACRO_start_file is at line 0. debug_line_header repeats the
    // primary source file as file 1, which GCC also uses here.
    macro_unit.start_file(0, 1);
    for (const auto &macro : macros) macro_unit.define(detail::source_line(*macro), detail::define_string(*macro));
    macro_unit.end_file();
    const std::vector<detail::SectionRelocation> macro_relocations{
        {*macro_unit.debug_line_offset_field(), line_index, source_unit}};
    const auto macro_offset = macro_unit.finish();

    using enum pepp::bts::AttributeForms;
    const auto abbrevs = pepp::bts::write_abbreviations(*file->section_data[abbrev_index], detail::unit_abbreviations);
    pepp::bts::CompileUnitWriter<E> unit(*file->section_data[info_index], address_size, static_cast<u32>(abbrevs));
    unit.entry(detail::unit_abbreviations[0].code);
    const std::vector<detail::SectionRelocation> info_relocations{
        {unit.abbrev_offset_field(), abbrev_index, abbrevs},
        {unit.value(DW_FORM_sec_offset, {.number = source_unit}), line_index, source_unit},
        {unit.value(DW_FORM_sec_offset, {.number = list}), rnglists_index, list},
        {unit.value(DW_FORM_sec_offset, {.number = macro_offset}), macro_index, macro_offset},
    };
    unit.value(DW_FORM_string, {.string = source_name});
    unit.value(DW_FORM_string, {.string = directory});
    unit.value(DW_FORM_string, {.string = debugProducer});
    unit.value(DW_FORM_data2, {.number = bits::to_underlying(pepp::bts::Languages::DW_LANG_Assembler)});
    unit.finish();

    if (file->header.e_type == bits::to_underlying(pepp::bts::ElfFileType::ET_REL)) {
      using detail::write_section_relocations;
      write_section_relocations<File::elf_bits, E>(*file, line_index, debugLineSection, address_relocation,
                                                   line_relocations);
      write_section_relocations<File::elf_bits, E>(*file, listing_index, debugListingLineSection, address_relocation,
                                                   listing_relocations);
      write_section_relocations<File::elf_bits, E>(*file, rnglists_index, debugRnglistsSection, address_relocation,
                                                   range_relocations);
      write_section_relocations<File::elf_bits, E>(*file, info_index, debugInfoSection, offset_relocation,
                                                   info_relocations);
      write_section_relocations<File::elf_bits, E>(*file, macro_index, debugMacroSection, offset_relocation,
                                                   macro_relocations);
    }
  };
  std::visit(f, elf);
}

// Decodes line info of elf, into one mapping per (section, address) in ascending order.
template <pepp::bts::ElfBits B, pepp::bts::ElfEndian E>
std::expected<std::vector<LineMapping>, std::string> read_line_mappings(const pepp::bts::PackedElf<B, E> &elf) {
  const auto line_str = pepp::bts::section_bytes(elf, pepp::bts::find_named_section(elf, debugLineStrSection));
  std::map<std::pair<u16, u64>, std::pair<u32, u32>> lines;
  const auto read_section = [&](const char *name, bool for_listing) -> std::expected<void, std::string> {
    const auto index = pepp::bts::find_named_section(elf, name);
    const auto data = pepp::bts::section_bytes(elf, index);
    const auto sections = pepp::bts::relocation_symbol_sections(elf, index);
    for (size_t offset = 0; offset < data.size();) {
      const auto unit = pepp::bts::parse_line_program_unit<E>(data, offset, line_str);
      if (!unit) return std::unexpected(unit.error());
      const auto program_start = static_cast<u64>(unit->program.data() - data.data());
      const auto on_row = [&](const pepp::bts::DebugLineState &row, std::optional<size_t> address_operand) {
        if (row.end_sequence) return;
        u16 section = bits::to_underlying(pepp::bts::SectionIndices::SHN_ABS);
        if (address_operand) {
          const auto it = sections.find(program_start + *address_operand);
          if (it != sections.cend()) section = it->second;
        }
        auto &[source_line, listing_line] = lines[{section, row.address}];
        (for_listing ? listing_line : source_line) = static_cast<u32>(row.line);
      };
      if (const auto ran = pepp::bts::run_line_program<E>(*unit, on_row); !ran) return std::unexpected(ran.error());
      offset = unit->next_unit;
    }
    return {};
  };
  if (const auto read = read_section(debugLineSection, false); !read) return std::unexpected(read.error());
  if (const auto read = read_section(debugListingLineSection, true); !read) return std::unexpected(read.error());
  std::vector<LineMapping> ret;
  ret.reserve(lines.size());
  for (const auto &[where, line] : lines)
    ret.push_back(
        {.section = where.first, .address = where.second, .source_line = line.first, .listing_line = line.second});
  return ret;
}
} // namespace pepp::tc
