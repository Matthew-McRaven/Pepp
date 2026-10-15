#include "core/langs/asmb/debug_line.hpp"
#include <map>

pepp::bts::LineProgramHeader pepp::tc::detail::debug_line_header(u8 address_size, std::string_view file) {
  pepp::bts::LineProgramHeader ret;
  ret.address_size = address_size;
  // Instructions and data directives may be any number of bytes long.
  ret.minimum_instruction_length = 1;
  ret.maximum_operations_per_instruction = 1;
  ret.default_is_stmt = true;
  ret.line_base = -5;
  ret.line_range = 14;
  ret.standard_opcode_lengths = {0, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1};
  ret.directories = {"."};
  ret.files = {{.path = std::string(file), .directory_index = 0}};
  return ret;
}

namespace {
template <pepp::bts::ElfEndian E>
std::expected<std::vector<pepp::tc::LineMapping>, std::string> read_units(bits::span<const u8> debug_line,
                                                                         bits::span<const u8> debug_line_str) {
  // Source and listing lines arrive in separate units, so merge them by address.
  std::map<u64, std::pair<u32, u32>> lines;
  for (size_t offset = 0; offset < debug_line.size();) {
    const auto unit = pepp::bts::parse_line_program_unit<E>(debug_line, offset, debug_line_str);
    if (!unit) return std::unexpected(unit.error());
    const bool listing = !unit->header.files.empty() && unit->header.files[0].path == pepp::tc::debugLineListingFile;
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
