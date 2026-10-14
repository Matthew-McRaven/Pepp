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
namespace pepp::bts {

/*
 * All of the following has been taken from the DWARF 5 standard, Section 6.2
 */
enum class StandardOpcodes : u8 {
  DW_LNS_copy = 1,
  DW_LNS_advance_pc = 2,
  DW_LNS_advance_line = 3,
  DW_LNS_set_file = 4,
  DW_LNS_set_column = 5,
  DW_LNS_negate_stmt = 6,
  DW_LNS_set_basic_block = 7,
  DW_LNS_const_add_pc = 8,
  DW_LNS_fixed_advance_pc = 9,
  DW_LNS_set_prologue_end = 10,
  DW_LNS_set_epilogue_begin = 11,
  DW_LNS_set_isa = 12,
};
enum class ExtendedOpcodes : u8 {
  DW_LNE_end_sequence = 1,
  DW_LNE_set_address = 2,
  DW_LNE_define_file = 3,
  DW_LNE_set_discriminator = 4,
};

#pragma pack(push, 1)
struct PackedDebugLineHeader {
  u32 length;
  u16 version;
  // Number of bytes
  i32 header_length;
  u8 min_instruction_length;
  u8 default_is_stmt;
  i8 line_base;
  u8 line_range;
  u8 opcode_base;
};
#pragma pack(pop)

struct FileEntry {
  std::string name;
  // The directory containing this file as an index into include_directories, encoded as ULEB128
  u32 directory_index = 0;
  // Implementation-defined timestamp for when the file was last modified, encoded as ULEB128. 0 if unknown.
  u32 last_modification_time = 0;
  // Size of file in bytes, encoded as ULEB128. 0 if unknown.
  u32 file_length = 0;
};

struct DebugLineHeader {
  PackedDebugLineHeader header;
  std::vector<u8> standard_opcode_lengths;
  // TODO: directory and file entries.
  // std::vector<std::string> include_directories;
  // std::vector<FileEntry> file_entries;
};

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