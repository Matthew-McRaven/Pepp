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
#include <cstddef>
#include "../../integers.h"
#include "core/formats/dwarf/enums.hpp"
#include "core/formats/dwarf/forms.hpp"
#include "core/formats/elf/packed_storage.hpp"
#include "core/formats/elf/packed_types.hpp"
namespace pepp::bts {

#pragma pack(push, 1)
// Header of a range list table, per Section 7.28 items 1-5.
template <ElfEndian E> struct PackedRangeListHeader {
  // Bytes following this field to the end of the table.
  U32<E> unit_length;
  U16<E> version;
  u8 address_size;
  u8 segment_selector_size;
  U32<E> offset_entry_count;
};
#pragma pack(pop)
static_assert(sizeof(PackedRangeListHeader<ElfEndian::le>) == 12);

// Construct a single line program into `out` in a streaming fashion. The header is written on construction and entries
// are emitted as needed. Out may have no concurrent writers until finish() is called.
// The table has no offset array, so units must refer to its lists with DW_FORM_sec_offset (Section 7.28).
template <ElfEndian E> class RangeListWriter {
public:
  RangeListWriter(AStorage &out, u8 address_size);

  static constexpr u16 version() noexcept { return 5; }
  // Offset where the next range list begins.
  size_t list_offset() const noexcept { return _out.size(); }
  // An address+uleb128 pair. See Section 2.17.3 item 8.
  // Returns the offset of the address operand in `out`, so that a caller can relocate it.
  size_t start_length(u64 start, u64 length);
  // Ends the current range list, see Section 2.17.3 item 1
  void end_list();
  // Patches unit_length, and returns the offset for the start of the table within out.
  size_t finish();

private:
  AStorage &_out;
  u8 _address_size;
  // Base address (offset) in out for this table.
  size_t _table;
};

template <ElfEndian E>
RangeListWriter<E>::RangeListWriter(AStorage &out, u8 address_size) : _out(out), _address_size(address_size) {
  PackedRangeListHeader<E> packed{};
  packed.version = version();
  packed.address_size = address_size;
  _table = _out.append(packed);
}

template <ElfEndian E> size_t RangeListWriter<E>::start_length(u64 start, u64 length) {
  _out.append(bits::to_underlying(RangeListEntries::DW_RLE_start_length));
  const auto ret = _out.size();
  detail::write_address<E>(_out, _address_size, start);
  _out.append_uleb(length);
  return ret;
}

template <ElfEndian E> void RangeListWriter<E>::end_list() {
  _out.append(bits::to_underlying(RangeListEntries::DW_RLE_end_of_list));
}

template <ElfEndian E> size_t RangeListWriter<E>::finish() {
  // Section 7.28 item 1: the length does not include the length field itself.
  constexpr auto length_end = offsetof(PackedRangeListHeader<E>, version);
  // Patch the length field now that the table is complete.
  _out.set(_table + offsetof(PackedRangeListHeader<E>, unit_length),
           U32<E>(static_cast<u32>(_out.size() - _table - length_end)));
  return _table;
}

} // namespace pepp::bts
