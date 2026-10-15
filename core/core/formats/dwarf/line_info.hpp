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
#include "../../integers.h"
#include "core/formats/dwarf/cursor.hpp"
#include "core/formats/dwarf/enums.hpp"
#include "core/formats/elf/packed_types.hpp"
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

struct DebugLineState {
  // Current instructions is a recommended breakpoint location to "represent" a line
  // Value at reset actually needs to be default_is_stmt.
  bool is_stmt = false;
  // Current instruction begins a basic block.
  bool basic_block = false;
  // Current address if the first byte after the end of a seuqnce of lines. Other info in the same row is not
  // meaningful.
  bool end_sequence = false;
  // Current address is the first address (of possibly many) where a function-entry breakpoint should be taken.
  bool prologue_end = false;
  // Current address is the last address (of possibly many) where a function-exit breakpoint should be taken.
  bool epilogue_begin = false;
  u8 discriminator = 0;
  // Only meaningful for VLIW architectures. Otherwise 0.
  u8 op_index = 0;
  // ABI-defined value that declares the applicable instruction set for the current instruction
  u16 isa = 0;
  // Program counter value
  u32 address = 0;
  // Which source file this row applies to,
  u32 file = 1;
  // 1-indexed, with 0 reserved for "no line info"
  u32 line = 1;
  // 1-indexed, with 0 reserved for "left edge" of a line
  u32 column = 0;
};

} // namespace pepp::bts