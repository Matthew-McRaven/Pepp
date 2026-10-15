#pragma once
#include <algorithm>
#include <expected>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include "core/formats/dwarf/line_program.hpp"
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

static inline const auto debugLineSection = ".debug_line";
static inline const auto debugLineStrSection = ".debug_line_str";

// .debug_line holds two DWARF 5 line program units. The first maps addresses to source lines, and the second maps the
// same addresses to listing lines. Each unit names a single file, so readers can tell them apart.
static inline const auto debugLineSourceFile = "source";
static inline const auto debugLineListingFile = "listing";

// One address's lines, merged from both units. Lines are 1-based, and 0 means the address has no such line.
struct LineMapping {
  u64 address = 0;
  u32 source_line = 0, listing_line = 0;
  bool operator==(const LineMapping &) const = default;
};

namespace detail {
pepp::bts::LineProgramHeader debug_line_header(u8 address_size, std::string_view file);
}

namespace detail {
// Row numbers are 0-indexed, while line numbers are 1-indexed, so 0 means "no line number".
inline u32 source_line(const LinearIR &line) {
  return line.source_interval.valid() ? 1 + line.source_interval.lower().row : 0;
}
inline u32 listing_line(const IR2ListingLineMap &listing, const LinearIR &line) {
  const auto it = listing.find(&line);
  return it != listing.cend() ? 1 + it->second : 0;
}
// A DW_LNE_set_address operand to relocate against its section's STT_SECTION symbol.
struct LineRelocation {
  size_t offset;
  u16 section;
  u64 addend;
};

// Writes .rela.debug_line. Each address is relocated against its section's STT_SECTION symbol.
template <pepp::bts::ElfBits B, pepp::bts::ElfEndian E>
void write_line_relocations(pepp::bts::PackedGrowableElfFile<B, E> &elf, u16 debug_line, u32 type,
                            const std::vector<LineRelocation> &relocations) {
  using namespace pepp::bts;
  u16 symtab = 0;
  for (u16 it = 1; it < elf.section_headers.size() && symtab == 0; ++it)
    if (elf.section_headers[it].sh_type == bits::to_underlying(SectionTypes::SHT_SYMTAB)) symtab = it;
  if (symtab == 0) throw std::logic_error("write_line_relocations: no .symtab to relocate against");
  // Collect the symbols before adding a section, since the reader holds references into the section headers.
  std::vector<u32> section_symbol(elf.section_headers.size(), 0);
  {
    PackedSymbolReader<B, E> symbols(elf, symtab);
    for (u32 it = 1; it < symbols.symbol_count(); ++it)
      if (const auto symbol = symbols.get_symbol(it); symbol.st_type() == SymbolType::STT_SECTION)
        section_symbol.at(symbol.st_shndx) = it;
  }
  const auto rela = add_named_rela(elf, ".rela.debug_line", symtab, debug_line);
  elf.section_headers[rela].sh_flags = bits::to_underlying(pepp::bts::SectionFlags::SHF_INFO_LINK);
  elf.section_headers[rela].sh_addralign = sizeof(word<B>);
  PackedRelocationWriter<B, E> writer(elf, rela);
  for (const auto &[offset, section, addend] : relocations) {
    const u32 symbol = section_symbol.at(section);
    if (symbol == 0) throw std::logic_error("write_line_relocations: section has no STT_SECTION symbol");
    writer.add_rela(offset, type, symbol, static_cast<sword<B>>(addend));
  }
}
} // namespace detail

// Appends .debug_line to `elf`, with a row for every line of `prog` that has an address and a line number.
// Each section gets its own sequence. In an ET_REL file, each sequence's address is relocated by `address_relocation`.
// Advances within a sequence are not relocated, which relies on the linker never relaxing code (no R_RISCV_RELAX).
// In an ET_REL file, must follow sections_to_elf: relocations name section symbols by their final .symtab index.
template <typename Address>
void write_debug_line(pepp::bts::AnyGrowableElf &elf, u8 address_size, u32 address_relocation,
                      const std::vector<std::pair<SectionDescriptor, IRProgram>> &prog,
                      const IRMemoryAddressTable<Address> &addrs, const IR2ListingLineMap &listing) {
  auto f = [&](auto &file) {
    if (!file) return;
    using File = std::remove_cvref_t<decltype(*file)>;
    constexpr auto E = File::elf_endian;
    const auto index = pepp::bts::add_named_section(*file, debugLineSection, pepp::bts::SectionTypes::SHT_PROGBITS);
    auto &out = *file->section_data[index];
    std::vector<detail::LineRelocation> relocations;
    const auto write_unit = [&](std::string_view name, bool for_listing) {
      pepp::bts::LineProgramWriter<E> writer(out, detail::debug_line_header(address_size, name));
      for (const auto &[desc, lines] : prog) {
        const auto first_operand = writer.address_operands().size();
        for (const auto &line : lines) {
          const auto addr_it = addrs.find(line.get());
          if (addr_it == addrs.cend()) continue;
          const u32 number = for_listing ? detail::listing_line(listing, *line) : detail::source_line(*line);
          if (number != 0) writer.row(addr_it->second.address, addr_it->second.size, 0, number);
        }
        // A linker places each section on its own, so a sequence must not continue into the next one.
        writer.end_sequence();
        const auto &operands = writer.address_operands();
        for (auto it = first_operand; it < operands.size(); ++it)
          relocations.push_back({operands[it].offset, desc.section_index, operands[it].address - desc.low_address});
      }
      writer.finish();
    };
    write_unit(debugLineSourceFile, false);
    write_unit(debugLineListingFile, true);
    if (file->header.e_type == bits::to_underlying(pepp::bts::ElfFileType::ET_REL))
      detail::write_line_relocations<File::elf_bits, E>(*file, index, address_relocation, relocations);
  };
  std::visit(f, elf);
}

// Numbers listing lines in program order, as a listing with `bytes_per_line` bytes of object code per row prints them.
// Lines hidden from the listing get no number.
IR2ListingLineMap number_listing_lines(const std::vector<std::pair<SectionDescriptor, IRProgram>> &prog,
                                       const ProgramObjectCodeResult &object_code, u32 bytes_per_line);

// Decodes .debug_line, as written by write_debug_line, into one mapping per address in ascending order.
// `debug_line_str` is only needed when a unit names its files with DW_FORM_line_strp.
std::expected<std::vector<LineMapping>, std::string> read_line_mappings(bits::span<const u8> debug_line,
                                                                       bits::span<const u8> debug_line_str,
                                                                       pepp::bts::ElfEndian endian);
} // namespace pepp::tc
