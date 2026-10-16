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
// Line number information (.debug_line) enumerated constants, per DWARF 5 Sections 6.2 and 7.22.

// Per: DWARF 5 Table 7.25
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

// Per: DWARF 5 Table 7.26
enum class ExtendedOpcodes : u8 {
  DW_LNE_end_sequence = 1,
  DW_LNE_set_address = 2,
  DW_LNE_set_discriminator = 4,
  DW_LNE_lo_user = 0x80,
  DW_LNE_hi_user = 0xff,
};

// Content type codes for directory and file entry formats.
// Per: DWARF 5 Sections 6.2.4.1 and 6.2.4.2, Table 7.27. Encoded as ULEB128 (Section 6.2.4 items 14 and 18).
enum class LineContentTypes : u64 {
  DW_LNCT_path = 0x1,
  DW_LNCT_directory_index = 0x2,
  DW_LNCT_timestamp = 0x3,
  DW_LNCT_size = 0x4,
  DW_LNCT_MD5 = 0x5,
  DW_LNCT_lo_user = 0x2000,
  DW_LNCT_hi_user = 0x3fff,
};
} // namespace pepp::bts
