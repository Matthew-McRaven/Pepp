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


#include "core/formats/dwarf/info.hpp"
#include <array>
#include <catch.hpp>

using namespace pepp::bts;

namespace {
std::vector<u8> bytes_of(const AStorage &s) {
  const auto bytes = s.get(0, s.size());
  return {bytes.begin(), bytes.end()};
}
} // namespace

TEST_CASE("DWARF compile unit writer", "[scope:dwarf][kind:unit][arch:*]") {
  using enum Tags;
  using enum AttributeNames;
  using enum AttributeForms;
  using enum ChildrenDetermination;
  // Every code below is less than 0x80, so its ULEB128 encoding is the code itself.
  const auto b = [](auto value) { return static_cast<u8>(bits::to_underlying(value)); };
  static constexpr std::array<AttributeSpecification, 3> unit_attributes{{
      {DW_AT_name, DW_FORM_string},
      {DW_AT_stmt_list, DW_FORM_sec_offset},
      {DW_AT_low_pc, DW_FORM_addr},
  }};
  static constexpr std::array<AttributeSpecification, 1> type_attributes{{{DW_AT_byte_size, DW_FORM_data1}}};
  const std::array<Abbreviation, 2> abbreviations{{
      {1, DW_TAG_compile_unit, DW_CHILDREN_yes, unit_attributes},
      {2, DW_TAG_base_type, DW_CHILDREN_no, type_attributes},
  }};

  SECTION("Write an abbreviations table") {
    BlockStorage s;
    CHECK(write_abbreviations(s, abbreviations) == 0);
    // Section 7.5.3: code, tag, children, (name, form) pairs ending in 0, 0; the table ends with code 0.
    const std::vector<u8> expected{
        0x01, b(DW_TAG_compile_unit), b(DW_CHILDREN_yes), b(DW_AT_name), b(DW_FORM_string), b(DW_AT_stmt_list),
        b(DW_FORM_sec_offset), b(DW_AT_low_pc), b(DW_FORM_addr), 0x00, 0x00,
        0x02, b(DW_TAG_base_type), b(DW_CHILDREN_no), b(DW_AT_byte_size), b(DW_FORM_data1), 0x00, 0x00,
        0x00,
    };
    CHECK(bytes_of(s) == expected);
    // Section 7.5.2: code 0 is reserved for null entries.
    CHECK_THROWS_AS(write_abbreviations(s, std::array<Abbreviation, 1>{{{0, DW_TAG_base_type, DW_CHILDREN_no}}}),
                    std::logic_error);
  }

  SECTION("Write a big-endian compile unit") {
    BlockStorage s;
    CompileUnitWriter<ElfEndian::be> unit(s, 2, 0x34);
    CHECK(unit.abbrev_offset_field() == offsetof(PackedCompileUnitHeader<ElfEndian::be>, debug_abbrev_offset));
    unit.entry(1);
    unit.value(DW_FORM_string, {.string = "a.pep"});
    CHECK(unit.value(DW_FORM_sec_offset, {.number = 0x10}) == 19);
    CHECK(unit.address(0x0100) == 23);
    unit.entry(2);
    unit.value(DW_FORM_data1, {.number = 1});
    unit.end_children();
    CHECK(unit.finish() == 0);
    const std::vector<u8> expected{
        // Section 7.5.1.1 items 1-5: unit_length (24), version, unit_type, address_size, debug_abbrev_offset.
        0x00, 0x00, 0x00, 0x18, 0x00, 0x05, b(UnitTypes::DW_UT_compile), 0x02, 0x00, 0x00, 0x00, 0x34,
        0x01, 'a', '.', 'p', 'e', 'p', 0x00, 0x00, 0x00, 0x00, 0x10, 0x01, 0x00, // compile unit
        0x02, 0x01,                                                                // base type
        0x00,                                                                      // null entry
    };
    CHECK(bytes_of(s) == expected);
  }
}
