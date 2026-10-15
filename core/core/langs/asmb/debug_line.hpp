#pragma once
#include <algorithm>
#include <expected>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include "core/compile/ir_linear/attr_section.hpp"
#include "core/formats/dwarf/line_program.hpp"
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
} // namespace detail

// Appends .debug_line to `elf`, with a row for every line of `prog` that has an address and a line number.
template <typename Address>
void write_debug_line(pepp::bts::AnyGrowableElf &elf, u8 address_size,
                      const std::vector<std::pair<SectionDescriptor, IRProgram>> &prog,
                      const IRMemoryAddressTable<Address> &addrs, const IR2ListingLineMap &listing) {
  auto f = [&](auto &file) {
    if (!file) return;
    constexpr auto E = std::remove_cvref_t<decltype(*file)>::elf_endian;
    const auto index = pepp::bts::add_named_section(*file, debugLineSection, pepp::bts::SectionTypes::SHT_PROGBITS);
    auto &out = *file->section_data[index];
    const auto write_unit = [&](std::string_view name, bool for_listing) {
      pepp::bts::LineProgramWriter<E> writer(out, detail::debug_line_header(address_size, name));
      for (const auto &sec : prog) {
        for (const auto &line : sec.second) {
          const auto addr_it = addrs.find(line.get());
          if (addr_it == addrs.cend()) continue;
          const u32 number = for_listing ? detail::listing_line(listing, *line) : detail::source_line(*line);
          if (number != 0) writer.row(addr_it->second.address, addr_it->second.size, 0, number);
        }
      }
      writer.finish();
    };
    write_unit(debugLineSourceFile, false);
    write_unit(debugLineListingFile, true);
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
