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
#include <expected>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include "../../integers.h"
#include "core/formats/byte_cursor.hpp"
#include "core/formats/dwarf/enums.hpp"
#include "core/formats/dwarf/forms.hpp"
#include "core/formats/elf/packed_storage.hpp"
#include "core/formats/elf/packed_types.hpp"
#include "core/math/bitmanip/span.hpp"
namespace pepp::bts {

// A define string split into its parts, per Section 6.3.2.2. Only a function-like macro has parameters.
struct MacroDefineString {
  std::string_view name;
  std::optional<std::vector<std::string_view>> parameters;
  std::string_view definition;
};

// Formats a function-like define string, writing the parameter list even when it is empty.
// Per: Section 6.3.2.2. No whitespace before "(" or between parameters, and exactly one space after ")".
std::string format_define(std::string_view name, bits::span<const std::string_view> parameters,
                          std::string_view definition);
// Splits a define string, or returns nullopt if its parameter list is unterminated.
std::optional<MacroDefineString> parse_define(std::string_view string);

// A define or undefine entry of a macro unit.
struct MacroEntry {
  // DW_MACRO_define or DW_MACRO_undef.
  MacroOpcodes opcode;
  u64 line;
  std::string_view string;
  // True within a DW_MACRO_start_file. Entries before the first one come from outside the source (Section 6.3.2.4).
  bool in_file;
  bool operator==(const MacroEntry &) const = default;
};

struct MacroUnit {
  std::optional<u64> debug_line_offset;
  std::vector<MacroEntry> entries;
  // Offset just past this unit's terminating entry.
  size_t next_unit;
};

// Parses the macro unit starting at `offset` within a .debug_macro section.
// Only DW_MACRO_define, DW_MACRO_undef, and the file inclusion entries are decoded. Other opcodes described by the
// opcode_operands_table (Section 6.3.1 item 4) are skipped, and the rest are errors.
template <ElfEndian E>
std::expected<MacroUnit, std::string> parse_macro_unit(bits::span<const u8> section, size_t offset);

// Streams one macro unit into .debug_macro: the header on construction, entries as they are written, and the
// terminating entry on finish(). Nothing else may be appended to `out` until finish() returns.
template <ElfEndian E> class MacroUnitWriter {
public:
  // With a debug_line_offset, sets debug_line_offset_flag and writes the field (Section 6.3.1 items 2 and 3).
  MacroUnitWriter(AStorage &out, std::optional<u32> debug_line_offset);

  // Section 7.23.
  static constexpr u16 version() noexcept { return 5; }
  // Offset in `out` of the header's debug_line_offset, if written, so that a caller can relocate it.
  std::optional<size_t> debug_line_offset_field() const noexcept { return _line_offset_field; }
  // Section 6.3.2.1 item 1. Line 0 is for macros not defined in the source text (Section 6.3.2.4).
  void define(u64 line, std::string_view string);
  // Section 6.3.3.1 item 1. `file` indexes the file table of the line program at debug_line_offset.
  void start_file(u64 line, u64 file);
  // Section 6.3.3.1 item 2.
  void end_file();
  // Appends the entry with opcode 0 that ends the unit (Section 6.3). Returns the offset of the unit within `out`.
  size_t finish();

private:
  AStorage &_out;
  size_t _unit;
  std::optional<size_t> _line_offset_field;
};

/*
 * Implementations
 */
template <ElfEndian E>
MacroUnitWriter<E>::MacroUnitWriter(AStorage &out, std::optional<u32> debug_line_offset) : _out(out) {
  using enum MacroHeaderFlags;
  _unit = _out.append(U16<E>(version()));
  // Section 6.3.1 items 1-3. offset_size_flag stays clear for the 32-bit format.
  _out.append(debug_line_offset ? bits::to_underlying(debug_line_offset_flag) : u8{0});
  if (debug_line_offset) _line_offset_field = _out.append(U32<E>(*debug_line_offset));
}

template <ElfEndian E> void MacroUnitWriter<E>::define(u64 line, std::string_view string) {
  _out.append(bits::to_underlying(MacroOpcodes::DW_MACRO_define));
  _out.append_uleb(line);
  _out.append_cstr(string);
}

template <ElfEndian E> void MacroUnitWriter<E>::start_file(u64 line, u64 file) {
  // Section 6.3.3.1 item 1: the header must then refer to .debug_line.
  if (!_line_offset_field) throw std::logic_error("MacroUnitWriter: start_file requires a debug_line_offset");
  _out.append(bits::to_underlying(MacroOpcodes::DW_MACRO_start_file));
  _out.append_uleb(line);
  _out.append_uleb(file);
}

template <ElfEndian E> void MacroUnitWriter<E>::end_file() {
  _out.append(bits::to_underlying(MacroOpcodes::DW_MACRO_end_file));
}

template <ElfEndian E> size_t MacroUnitWriter<E>::finish() {
  _out.append(u8{0});
  return _unit;
}

template <ElfEndian E>
std::expected<MacroUnit, std::string> parse_macro_unit(bits::span<const u8> section, size_t offset) {
  using enum MacroOpcodes;
  using enum MacroHeaderFlags;
  if (offset >= section.size()) return std::unexpected("unit offset is past the end of the section");
  ByteCursor c{section.subspan(offset)};
  // Section 6.3.1 items 1 and 2.
  if (const u16 version = c.fixed<U16<E>>(); c.ok() && version != MacroUnitWriter<E>::version())
    return std::unexpected("unsupported macro information version " + std::to_string(version));
  const u8 flags = c.fixed<u8>();
  if (!c.ok()) return std::unexpected("section too small for a macro information header");
  if (flags & bits::to_underlying(offset_size_flag)) return std::unexpected("64-bit DWARF is not supported");

  MacroUnit ret;
  if (flags & bits::to_underlying(debug_line_offset_flag)) ret.debug_line_offset = c.fixed<U32<E>>();
  // Section 6.3.1 item 4: a count, then each opcode with its operands' forms.
  std::map<u8, std::vector<AttributeForms>> operands;
  if (flags & bits::to_underlying(opcode_operands_table_flag)) {
    const u8 count = c.fixed<u8>();
    for (u8 it = 0; it < count && c.ok(); ++it) {
      auto &forms = operands[c.fixed<u8>()];
      forms.clear();
      for (u64 operand = 0, n = c.uleb(); operand < n && c.ok(); ++operand)
        forms.push_back(AttributeForms(c.fixed<u8>()));
    }
  }

  u64 depth = 0;
  while (c.ok()) {
    const u8 opcode = c.fixed<u8>();
    if (!c.ok()) break;
    // Section 6.3: the unit ends with an entry whose opcode is 0.
    if (opcode == 0) {
      ret.next_unit = offset + c.offset;
      return ret;
    }
    switch (MacroOpcodes(opcode)) {
    // Section 6.3.2.1 item 1.
    case DW_MACRO_define: [[fallthrough]];
    case DW_MACRO_undef: {
      const auto line = c.uleb();
      ret.entries.push_back({MacroOpcodes(opcode), line, c.cstr(), depth > 0});
      break;
    }
    // Section 6.3.3.1.
    case DW_MACRO_start_file:
      c.uleb(), c.uleb();
      ++depth;
      break;
    case DW_MACRO_end_file:
      if (depth == 0) return std::unexpected("DW_MACRO_end_file without a DW_MACRO_start_file");
      --depth;
      break;
    default:
      if (const auto it = operands.find(opcode); it != operands.end())
        for (const auto form : it->second) read_form<E>(c, form);
      else return std::unexpected("unsupported macro information opcode " + std::to_string(opcode));
    }
  }
  return std::unexpected(c.error ? c.error : "macro unit has no terminating entry");
}

} // namespace pepp::bts
