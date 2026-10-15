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


#include "core/formats/dwarf/rnglists.hpp"
#include <catch.hpp>

using namespace pepp::bts;

TEST_CASE("DWARF range list writer", "[scope:dwarf][kind:unit][arch:*]") {
  using enum RangeListEntries;
  const auto b = [](auto value) { return static_cast<u8>(bits::to_underlying(value)); };
  BlockStorage s;
  RangeListWriter<ElfEndian::le> table(s, 4);
  CHECK(table.list_offset() == sizeof(PackedRangeListHeader<ElfEndian::le>));
  CHECK(table.start_length(0x1000, 0x10) == 13);
  CHECK(table.start_length(0x2000, 0x200) == 19);
  table.end_list();
  CHECK(table.finish() == 0);
  // clang-format off
  const std::vector<u8> expected{
      //Section 7.28 items 1-5. unit_length (22), version, address_size, segment_selector_size, offset_entry_count.
      0x16, 0x00, 0x00, 0x00, 0x05, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
      // Each entry is its (kind, operand) pairs ; addresses are 4 bytes, then ULEB values. ULEB128 0x200 is 0x80 0x04.
      b(DW_RLE_start_length), 0x00, 0x10, 0x00, 0x00, 0x10,
      b(DW_RLE_start_length), 0x00, 0x20, 0x00, 0x00, 0x80, 0x04,
      b(DW_RLE_end_of_list),
  };
  // clang-format on
  const auto bytes = s.get(0, s.size());
  CHECK(std::vector<u8>(bytes.begin(), bytes.end()) == expected);
}
