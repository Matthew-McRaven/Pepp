#include "core/langs/asmb/debug_line.hpp"
#include <map>
#include "core/compile/ir_linear/attr_comment.hpp"

pepp::bts::LineProgramHeader pepp::tc::detail::debug_line_header(u8 address_size, std::string_view directory,
                                                                  std::string_view file) {
  pepp::bts::LineProgramHeader ret;
  ret.address_size = address_size;
  // Instructions and data directives may be any number of bytes long.
  ret.minimum_instruction_length = 1;
  ret.maximum_operations_per_instruction = 1;
  ret.default_is_stmt = true;
  ret.line_base = -5;
  ret.line_range = 14;
  ret.standard_opcode_lengths = {0, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1};
  // Section 6.2.4 item 16: entry 0 is the current directory of the compilation.
  ret.directories = {std::string(directory)};
  // Section 6.2.4 item 20 makes entry 0 the primary source file. Repeating it as entry 1, as GCC does, matches the
  // file register's initial value of 1 (Table 6.4), so rows need no DW_LNS_set_file and binutils finds file 1.
  ret.files = {{.path = std::string(file), .directory_index = 0}, {.path = std::string(file), .directory_index = 0}};
  return ret;
}

pepp::tc::IR2ListingLineMap
pepp::tc::number_listing_lines(const std::vector<std::pair<SectionDescriptor, IRProgram>> &prog,
                               const ProgramObjectCodeResult &object_code, u32 bytes_per_line) {
  IR2ListingLineMap ret;
  u32 listing_number = 0;
  for (const auto &sec : prog) {
    for (const auto &line : sec.second) {
      if (hidden_from_listing(*line)) continue;
      const auto oc_it = object_code.ir_to_object_code.find(line.get());
      const size_t size = oc_it == object_code.ir_to_object_code.cend() ? 0 : oc_it->second.size();
      ret.container.emplace_back(IR2ListingLinePair{line.get(), listing_number});
      // The first row always prints, even without object code. Each further row continues the object code.
      listing_number += std::max<u32>(1, static_cast<u32>((size + bytes_per_line - 1) / bytes_per_line));
    }
  }
  std::sort(ret.container.begin(), ret.container.end(), IR2ListingLineComparator{});
  return ret;
}

namespace {
template <pepp::bts::ElfEndian E>
std::expected<std::vector<pepp::tc::LineMapping>, std::string> read_units(bits::span<const u8> debug_line,
                                                                         bits::span<const u8> debug_line_str) {
  // Source and listing lines arrive in separate units, so merge them by address.
  std::map<u64, std::pair<u32, u32>> lines;
  for (size_t offset = 0, index = 0; offset < debug_line.size(); ++index) {
    const auto unit = pepp::bts::parse_line_program_unit<E>(debug_line, offset, debug_line_str);
    if (!unit) return std::unexpected(unit.error());
    // write_debug_line writes the source unit, then the listing unit. Their file names may be anything.
    const bool listing = index == 1;
    const auto ran = pepp::bts::run_line_program<E>(*unit, [&](const pepp::bts::DebugLineState &row) {
      if (row.end_sequence) return;
      auto &[source_line, listing_line] = lines[row.address];
      (listing ? listing_line : source_line) = static_cast<u32>(row.line);
    });
    if (!ran) return std::unexpected(ran.error());
    offset = unit->next_unit;
  }
  std::vector<pepp::tc::LineMapping> ret;
  ret.reserve(lines.size());
  for (const auto &[address, line] : lines)
    ret.push_back({.address = address, .source_line = line.first, .listing_line = line.second});
  return ret;
}
} // namespace

std::expected<std::vector<pepp::tc::LineMapping>, std::string>
pepp::tc::read_line_mappings(bits::span<const u8> debug_line, bits::span<const u8> debug_line_str,
                             pepp::bts::ElfEndian endian) {
  if (endian == pepp::bts::ElfEndian::le) return read_units<pepp::bts::ElfEndian::le>(debug_line, debug_line_str);
  return read_units<pepp::bts::ElfEndian::be>(debug_line, debug_line_str);
}
