/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
 *
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

#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <expected>
#include <string>
#include <vector>
#include "../../integers.h"
#include "core/formats/dwarf/cursor.hpp"
#include "core/formats/dwarf/enums.hpp"
#include "core/formats/dwarf/forms.hpp"
#include "core/formats/elf/packed_types.hpp"
#include "core/math/bitmanip/leb128.hpp"
namespace pepp::bts {

#pragma pack(push, 1)
// Fixed-size prefix of a line program header, per DWARF 5 Section 6.2.4 items 1-11. Only 32-bit DWARF is supported.
// Variable-length fields (opcode lengths, directory and file tables) follow it.
template <ElfEndian E> struct PackedLineProgramHeader {
  // Bytes following this field to the end of the unit.
  U32<E> unit_length;
  U16<E> version;
  u8 address_size;
  u8 segment_selector_size;
  // Bytes following this field to the first opcode.
  U32<E> header_length;
  u8 minimum_instruction_length;
  u8 maximum_operations_per_instruction;
  u8 default_is_stmt;
  i8 line_base;
  u8 line_range;
  u8 opcode_base;
};
#pragma pack(pop)
static_assert(sizeof(PackedLineProgramHeader<ElfEndian::le>) == 18);

// Item 1's and item 5's lengths are measured from the end of their own fields; neither depends on byte order.
inline constexpr size_t line_unit_length_end = offsetof(PackedLineProgramHeader<ElfEndian::le>, version);
inline constexpr size_t line_header_length_end =
    offsetof(PackedLineProgramHeader<ElfEndian::le>, minimum_instruction_length);

// Per: DWARF 5 Section 6.2.4 items 14 and 18.
struct EntryFormat {
  LineContentTypes content;
  AttributeForms form;
};

// Per: DWARF 5 Section 6.2.4 item 20 and Section 6.2.4.1. Fields other than these two are skipped on read.
struct LineFileEntry {
  std::string path;
  // Index into LineProgramHeader::directories.
  u64 directory_index = 0;
};

// Unpacked line program header, per DWARF 5 Section 6.2.4.
// Fields derivable from the others (version, opcode_base, header_length) are computed rather than stored.
struct LineProgramHeader {
  // The entry formats we write to, selected to have no cross-section dependencies.
  static constexpr std::array<EntryFormat, 1> directory_entry_format{{
      {LineContentTypes::DW_LNCT_path, AttributeForms::DW_FORM_string},
  }};
  static constexpr std::array<EntryFormat, 2> file_name_entry_format{{
      {LineContentTypes::DW_LNCT_path, AttributeForms::DW_FORM_string},
      {LineContentTypes::DW_LNCT_directory_index, AttributeForms::DW_FORM_udata},
  }};

  u8 address_size = 0;
  u8 segment_selector_size = 0;
  u8 minimum_instruction_length = 0;
  u8 maximum_operations_per_instruction = 0;
  bool default_is_stmt = false;
  i8 line_base = 0;
  u8 line_range = 0;
  // Item 12: LEB128 operand counts for opcodes [1, opcode_base).
  std::vector<u8> standard_opcode_lengths;
  // Item 16: entry 0 is the compilation directory.
  std::vector<std::string> directories;
  // Item 20: entry 0 is the primary source file.
  std::vector<LineFileEntry> files;

  // Item 2, per Section 7.22.
  static constexpr u16 version() noexcept { return 5; }
  // Item 11: item 12 has opcode_base - 1 entries, starting at opcode 1.
  // Opcode 0 is not a standard opcode; Section 6.2.3 uses a zero byte to introduce an extended opcode.
  u8 opcode_base() const noexcept { return static_cast<u8>(standard_opcode_lengths.size() + 1); }
  // Item 5: bytes from the end of the header_length field through item 20, when encoded with the formats above.
  // A parsed header may have used other formats, so this need not match the bytes it was parsed from.
  u32 header_length() const noexcept;
};

struct LineProgramUnit {
  LineProgramHeader header;
  // The opcodes of the line program.
  bits::span<const u8> program;
  // Offset of the following unit within this section.
  size_t next_unit = 0;

  // Item 1: bytes from the end of the unit_length field through the end of the program.
  u32 unit_length() const noexcept {
    return static_cast<u32>(line_header_length_end - line_unit_length_end + header.header_length() + program.size());
  }
};

namespace detail {
// The value written for one field of a directory or file entry. Our directory format has only a path.
inline FormValue entry_value(LineContentTypes content, std::string_view path, u64 directory_index) {
  switch (content) {
  case LineContentTypes::DW_LNCT_path: return {.string = path};
  case LineContentTypes::DW_LNCT_directory_index: return {.number = directory_index};
  default: throw std::logic_error("entry_value: content type is not written");
  }
}
} // namespace detail

inline u32 LineProgramHeader::header_length() const noexcept {
  // Items 13-14 and 17-18: a ubyte count, then a pair of ULEB128s per format.
  const auto formats_size = [](bits::span<const EntryFormat> formats) {
    size_t ret = 1;
    for (const auto &[content, form] : formats)
      ret += bits::getULEB128Size(bits::to_underlying(content)) + bits::getULEB128Size(bits::to_underlying(form));
    return ret;
  };
  const auto entry_size = [](bits::span<const EntryFormat> formats, std::string_view path, u64 directory_index) {
    size_t ret = 0;
    for (const auto &[content, form] : formats) ret += form_size(form, detail::entry_value(content, path, directory_index));
    return ret;
  };
  // Items 6-11 are fixed-size, and item 12 is one ubyte per entry.
  size_t ret = sizeof(PackedLineProgramHeader<ElfEndian::le>) - line_header_length_end;
  ret += standard_opcode_lengths.size();
  // Items 13-16.
  ret += formats_size(directory_entry_format) + bits::getULEB128Size(directories.size());
  for (const auto &directory : directories) ret += entry_size(directory_entry_format, directory, 0);
  // Items 17-20.
  ret += formats_size(file_name_entry_format) + bits::getULEB128Size(files.size());
  for (const auto &file : files) ret += entry_size(file_name_entry_format, file.path, file.directory_index);
  return static_cast<u32>(ret);
}

// Appends a line program header items 1-20. Returns the offset of the header.
// The caller must provide the compilation directory (header.directories[0]) and the primary source file
// (header.files[0]). The caller must also provide the unit_length per Section 6.2.4.
template <ElfEndian E> size_t write_line_program_header(AStorage &out, const LineProgramHeader &header, u32 unit_length) {
  // Items 1-11.
  PackedLineProgramHeader<E> packed{};
  packed.unit_length = unit_length;
  packed.version = LineProgramHeader::version();
  packed.address_size = header.address_size;
  packed.segment_selector_size = header.segment_selector_size;
  packed.header_length = header.header_length();
  packed.minimum_instruction_length = header.minimum_instruction_length;
  packed.maximum_operations_per_instruction = header.maximum_operations_per_instruction;
  packed.default_is_stmt = header.default_is_stmt ? 1 : 0;
  packed.line_base = header.line_base;
  packed.line_range = header.line_range;
  packed.opcode_base = header.opcode_base();
  const auto ret = out.append(packed);
  // Item 12.
  out.append(bits::span<const u8>{header.standard_opcode_lengths});

  const auto write_formats = [&](bits::span<const EntryFormat> formats) {
    out.append(static_cast<u8>(formats.size()));
    for (const auto &[content, form] : formats) {
      out.append_uleb(bits::to_underlying(content));
      out.append_uleb(bits::to_underlying(form));
    }
  };
  const auto write_entry = [&](bits::span<const EntryFormat> formats, std::string_view path, u64 directory_index) {
    for (const auto &[content, form] : formats) write_form<E>(out, form, detail::entry_value(content, path, directory_index));
  };
  // Items 13-16.
  write_formats(LineProgramHeader::directory_entry_format);
  out.append_uleb(header.directories.size());
  for (const auto &directory : header.directories) write_entry(LineProgramHeader::directory_entry_format, directory, 0);
  // Items 17-20.
  write_formats(LineProgramHeader::file_name_entry_format);
  out.append_uleb(header.files.size());
  for (const auto &file : header.files)
    write_entry(LineProgramHeader::file_name_entry_format, file.path, file.directory_index);
  return ret;
}

// Appends a complete line program unit: its header followed by `unit.program`. Returns the offset of the unit.
template <ElfEndian E> size_t write_line_program_unit(AStorage &out, const LineProgramUnit &unit) {
  const auto ret = write_line_program_header<E>(out, unit.header, unit.unit_length());
  out.append(unit.program);
  return ret;
}

namespace detail {
// Reads a directory count+entries, followed by File count+entries.
// Per: DWARF 5 Section 6.2.4 items 13-16 (directories) and 17-20 (files).
template <ElfEndian E> std::vector<LineFileEntry> read_entries(DwarfCursor &c, bits::span<const u8> line_str) {
  using enum LineContentTypes;
  // Section 6.2.4.1: a DW_FORM_line_strp path is an offset to a null-terminated string in .debug_line_str.
  const auto line_string = [&](u64 offset) -> std::string_view {
    if (offset >= line_str.size()) {
      c.fail("DW_FORM_line_strp offset is past the end of .debug_line_str");
      return {};
    }
    DwarfCursor strings{line_str.subspan(offset)};
    const auto ret = strings.cstr();
    if (!strings.ok()) c.fail(strings.error);
    return ret;
  };
  std::vector<EntryFormat> formats(c.fixed<u8>());
  for (auto &format : formats) {
    format.content = LineContentTypes(c.uleb());
    format.form = AttributeForms(c.uleb());
  }
  const auto count = c.uleb();
  // Not required by the standard, but guarantees each entry consumes input, so a corrupt count cannot spin.
  const bool has_path = std::ranges::any_of(formats, [](const auto &f) { return f.content == DW_LNCT_path; });
  if (count != 0 && !has_path) c.fail("entry format lacks DW_LNCT_path");

  std::vector<LineFileEntry> ret;
  for (u64 it = 0; it < count && c.ok(); ++it) {
    auto &entry = ret.emplace_back();
    for (const auto &format : formats) {
      const auto value = read_form<E>(c, format.form);
      if (format.content == DW_LNCT_path) {
        // Section 6.2.4.1 also allows DW_FORM_strp and DW_FORM_strp_sup, whose strings live in sections we do not read.
        if (format.form == AttributeForms::DW_FORM_string) entry.path = value.string;
        else if (format.form == AttributeForms::DW_FORM_line_strp) entry.path = line_string(value.number);
        else c.fail("unsupported form for DW_LNCT_path");
      } else if (format.content == DW_LNCT_directory_index) entry.directory_index = value.number;
    }
  }
  return ret;
}
} // namespace detail

// Parses the line program unit starting at `offset` within a .debug_line section.
// `line_str` is the .debug_line_str section, which is only needed for DW_FORM_line_strp paths.
// Only DWARF5's  32-bit format is supported.
template <ElfEndian E>
std::expected<LineProgramUnit, std::string> parse_line_program_unit(bits::span<const u8> section, size_t offset,
                                                                    bits::span<const u8> line_str = {}) {
  using Packed = PackedLineProgramHeader<E>;
  if (offset >= section.size()) return std::unexpected("unit offset is past the end of the section");
  DwarfCursor c{section.subspan(offset)};
  const auto packed = c.fixed<Packed>();
  if (!c.ok()) return std::unexpected("section too small for a line program header");
  // Section 7.2.2 reserves 0xfffffff0 through 0xffffffff. Section 7.4 item 1 uses 0xffffffff to introduce a 64-bit
  // length.
  const u32 unit_length = packed.unit_length;
  if (unit_length == 0xffffffff) return std::unexpected("64-bit DWARF is not supported");
  else if (unit_length >= 0xfffffff0) return std::unexpected("reserved unit_length value");
  const size_t unit_end = line_unit_length_end + unit_length;
  if (unit_end > c.data.size()) return std::unexpected("unit_length extends past the end of the section");
  if (const u16 version = packed.version; version != LineProgramHeader::version())
    return std::unexpected("unsupported line program version " + std::to_string(version));
  const size_t program_start = line_header_length_end + packed.header_length;
  if (program_start > unit_end) return std::unexpected("header_length extends past the end of the unit");
  // Item 12 has opcode_base - 1 entries.
  if (packed.opcode_base == 0) return std::unexpected("opcode_base must be at least 1");
  // Section 6.2.5.1: special opcodes take the remainder modulo line_range.
  if (packed.line_range == 0) return std::unexpected("line_range must be nonzero");

  LineProgramUnit ret;
  auto &header = ret.header;
  header.address_size = packed.address_size;
  header.segment_selector_size = packed.segment_selector_size;
  header.minimum_instruction_length = packed.minimum_instruction_length;
  header.maximum_operations_per_instruction = packed.maximum_operations_per_instruction;
  // Section 7.22: any nonzero byte is true.
  header.default_is_stmt = packed.default_is_stmt != 0;
  header.line_base = packed.line_base;
  header.line_range = packed.line_range;

  // Bound the remaining reads by header_length so a malformed table cannot spill into the program.
  c.data = c.data.first(program_start);
  const auto lengths = c.bytes(packed.opcode_base - 1);
  header.standard_opcode_lengths.assign(lengths.begin(), lengths.end());
  for (auto &directory : detail::read_entries<E>(c, line_str))
    header.directories.emplace_back(std::move(directory.path));
  header.files = detail::read_entries<E>(c, line_str);
  if (!c.ok()) return std::unexpected(c.error);

  ret.program = section.subspan(offset + program_start, unit_end - program_start);
  ret.next_unit = offset + unit_end;
  return ret;
}

} // namespace pepp::bts