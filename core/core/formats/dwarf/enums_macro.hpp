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
// Macro information (.debug_macro) enumerated constants, per Sections 6.3 and 7.23.

// Encoded as a single unsigned byte (Section 7.23). Defined in Table 7.28
enum class MacroOpcodes : u8 {
  DW_MACRO_define = 0x01,
  DW_MACRO_undef = 0x02,
  DW_MACRO_start_file = 0x03,
  DW_MACRO_end_file = 0x04,
  DW_MACRO_define_strp = 0x05,
  DW_MACRO_undef_strp = 0x06,
  DW_MACRO_import = 0x07,
  DW_MACRO_define_sup = 0x08,
  DW_MACRO_undef_sup = 0x09,
  DW_MACRO_import_sup = 0x0a,
  DW_MACRO_define_strx = 0x0b,
  DW_MACRO_undef_strx = 0x0c,
  DW_MACRO_lo_user = 0xe0,
  DW_MACRO_hi_user = 0xff,
};

// Bits of the macro information header's flags field, beginning with the least significant bit per Section 6.3.1
// item 2.
enum class MacroHeaderFlags : u8 {
  offset_size_flag = 0x01,
  debug_line_offset_flag = 0x02,
  opcode_operands_table_flag = 0x04,
};
} // namespace pepp::bts
