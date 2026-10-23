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
#include <stdexcept>
#include <string_view>
#include "../../integers.h"
#include "core/formats/byte_cursor.hpp"
#include "core/formats/dwarf/enums_forms.hpp"
#include "core/formats/elf/packed_storage.hpp"
#include "core/formats/elf/packed_types.hpp"
#include "core/math/bitmanip/leb128.hpp"
namespace pepp::bts {

// A value decoded from its attribute form. Which member is meaningful depends on the form's class.
struct FormValue {
  u64 number = 0;
  std::string_view string;
};

// Decodes one value of the given form. Fixed-size data is in the ELF's byte order.
// Only forms that are self-contained are supported. Others report an error on the cursor.
template <ElfEndian E> FormValue read_form(ByteCursor &c, AttributeForms form) {
  using enum AttributeForms;
  FormValue ret;
  switch (form) {
  case DW_FORM_string: ret.string = c.cstr(); break;
  case DW_FORM_data1: ret.number = c.fixed<u8>(); break;
  case DW_FORM_data2: ret.number = c.fixed<U16<E>>(); break;
  case DW_FORM_data4: ret.number = c.fixed<U32<E>>(); break;
  case DW_FORM_data8: ret.number = c.fixed<U64<E>>(); break;
  case DW_FORM_udata: ret.number = c.uleb(); break;
  case DW_FORM_sdata: ret.number = static_cast<u64>(c.sleb()); break;
  // Section 7.5.5: present, with no value encoded in the entry itself.
  case DW_FORM_flag_present: ret.number = 1; break;
  // Section 7.4 item 3: 4 bytes in the 32-bit DWARF format.
  case DW_FORM_sec_offset: ret.number = c.fixed<U32<E>>(); break;
  // Section 7.5.5: a 4-byte offset from the first byte of the containing unit's header.
  case DW_FORM_ref4: ret.number = c.fixed<U32<E>>(); break;
  // An offset into .debug_line_str, which the caller resolves. Section 6.2.4.1: 4 bytes in the 32-bit DWARF format.
  case DW_FORM_line_strp: ret.number = c.fixed<U32<E>>(); break;
  // Consumed but not currently decoded.
  case DW_FORM_data16: c.bytes(16); break;
  case DW_FORM_block: c.bytes(c.uleb()); break;
  default: c.fail("unsupported attribute form"); break;
  }
  return ret;
}

inline size_t form_size(AttributeForms form, const FormValue &value) {
  using enum AttributeForms;
  switch (form) {
  case DW_FORM_string: return value.string.size() + 1;
  case DW_FORM_data1: return 1;
  case DW_FORM_data2: return 2;
  case DW_FORM_data4: return 4;
  case DW_FORM_data8: return 8;
  case DW_FORM_udata: return bits::getULEB128Size(value.number);
  case DW_FORM_sdata: return bits::getSLEB128Size(static_cast<i64>(value.number));
  case DW_FORM_flag_present: return 0;
  case DW_FORM_sec_offset: return 4;
  case DW_FORM_ref4: return 4;
  default: throw std::logic_error("form_size: unsupported attribute form");
  }
}

// Serialize a single value. Fixed-size data is written in the ELF's byte order.
template <ElfEndian E> void write_form(AStorage &out, AttributeForms form, const FormValue &value) {
  using enum AttributeForms;
  switch (form) {
  case DW_FORM_string: out.append_cstr(value.string); break;
  case DW_FORM_data1: out.append(static_cast<u8>(value.number)); break;
  case DW_FORM_data2: out.append(U16<E>(static_cast<u16>(value.number))); break;
  case DW_FORM_data4: out.append(U32<E>(static_cast<u32>(value.number))); break;
  case DW_FORM_data8: out.append(U64<E>(value.number)); break;
  case DW_FORM_udata: out.append_uleb(value.number); break;
  case DW_FORM_sdata: out.append_sleb(static_cast<i64>(value.number)); break;
  case DW_FORM_flag_present: break;
  case DW_FORM_sec_offset: out.append(U32<E>(static_cast<u32>(value.number))); break;
  case DW_FORM_ref4: out.append(U32<E>(static_cast<u32>(value.number))); break;
  default: throw std::logic_error("write_form: unsupported attribute form");
  }
}

// Addresses are size bytes wide
namespace detail {
template <ElfEndian E> void write_address(AStorage &out, u8 size, u64 address) {
  switch (size) {
  case 1: out.append(static_cast<u8>(address)); break;
  case 2: out.append(U16<E>(static_cast<u16>(address))); break;
  case 4: out.append(U32<E>(static_cast<u32>(address))); break;
  case 8: out.append(U64<E>(address)); break;
  default: throw std::logic_error("write_address: unsupported address_size");
  }
}

template <ElfEndian E> u64 read_address(ByteCursor &c, u8 size) {
  switch (size) {
  case 1: return c.fixed<u8>();
  case 2: return c.fixed<U16<E>>();
  case 4: return c.fixed<U32<E>>();
  case 8: return c.fixed<U64<E>>();
  default: c.fail("unsupported address_size"); return 0;
  }
}
} // namespace detail

} // namespace pepp::bts
