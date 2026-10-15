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

#include "core/formats/dwarf/line_info.hpp"
#include <array>
#include <catch.hpp>
#include "core/formats/elf/packed_storage.hpp"

using namespace pepp::bts;

namespace {
const std::array<u8, 1> copy_program{bits::to_underlying(StandardOpcodes::DW_LNS_copy)};

// A Pep/10-shaped unit: 2-byte addresses, one directory, one file.
LineProgramUnit pep10_unit() {
  LineProgramUnit unit;
  auto &header = unit.header;
  header.address_size = 2;
  header.minimum_instruction_length = 1;
  header.maximum_operations_per_instruction = 1;
  header.default_is_stmt = true;
  header.line_base = -5;
  header.line_range = 14;
  header.standard_opcode_lengths = {0, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1};
  header.directories = {"/"};
  header.files = {{.path = "a.pep", .directory_index = 0}};
  unit.program = copy_program;
  return unit;
}

template <ElfEndian E> std::vector<u8> write_unit(const LineProgramUnit &unit) {
  BlockStorage s;
  CHECK(write_line_program_unit<E>(s, unit) == 0);
  const auto bytes = s.get(0, s.size());
  return {bytes.begin(), bytes.end()};
}

template <ElfEndian E> void check_round_trip() {
  const auto unit = pep10_unit();
  const auto bytes = write_unit<E>(unit);
  CHECK(bytes.size() == line_unit_length_end + unit.unit_length());
  const auto parsed = parse_line_program_unit<E>(bytes, 0);
  REQUIRE(parsed.has_value());
  const auto &header = parsed->header;
  CHECK(header.address_size == unit.header.address_size);
  CHECK(header.segment_selector_size == unit.header.segment_selector_size);
  CHECK(header.minimum_instruction_length == unit.header.minimum_instruction_length);
  CHECK(header.maximum_operations_per_instruction == unit.header.maximum_operations_per_instruction);
  CHECK(header.default_is_stmt == unit.header.default_is_stmt);
  CHECK(header.line_base == unit.header.line_base);
  CHECK(header.line_range == unit.header.line_range);
  CHECK(header.standard_opcode_lengths == unit.header.standard_opcode_lengths);
  CHECK(header.opcode_base() == unit.header.opcode_base());
  CHECK(header.directories == unit.header.directories);
  REQUIRE(header.files.size() == 1);
  CHECK(header.files[0].path == unit.header.files[0].path);
  CHECK(header.files[0].directory_index == unit.header.files[0].directory_index);
  CHECK(std::ranges::equal(parsed->program, unit.program));
  CHECK(parsed->next_unit == bytes.size());
}

// A big-endian unit whose file entries use formats our current writer does no.
// i.e., DW_FORM_data1 for the directory index, and a DW_LNCT_MD5 field the parser must skip.
// Laid out per DWARF 5
// Section 6.2.4.
std::vector<u8> foreign_format_unit() {
  using enum LineContentTypes;
  using enum AttributeForms;
  using Packed = PackedLineProgramHeader<ElfEndian::be>;
  const auto header = pep10_unit().header;
  BlockStorage s;
  // Items 1-11; lengths are patched below.
  Packed packed{};
  packed.version = LineProgramHeader::version();
  packed.address_size = header.address_size;
  packed.minimum_instruction_length = header.minimum_instruction_length;
  packed.maximum_operations_per_instruction = header.maximum_operations_per_instruction;
  packed.default_is_stmt = header.default_is_stmt;
  packed.line_base = header.line_base;
  packed.line_range = header.line_range;
  packed.opcode_base = header.opcode_base();
  s.append(packed);
  // Item 12
  s.append(bits::span<const u8>{header.standard_opcode_lengths});
  // Items 13-16
  s.append(u8{1});
  s.append_uleb(bits::to_underlying(DW_LNCT_path));
  s.append_uleb(bits::to_underlying(DW_FORM_string));
  s.append_uleb(1);
  s.append_cstr("/src");
  // Items 17-20
  s.append(u8{3});
  s.append_uleb(bits::to_underlying(DW_LNCT_path));
  s.append_uleb(bits::to_underlying(DW_FORM_string));
  s.append_uleb(bits::to_underlying(DW_LNCT_directory_index));
  s.append_uleb(bits::to_underlying(DW_FORM_data1));
  s.append_uleb(bits::to_underlying(DW_LNCT_MD5));
  s.append_uleb(bits::to_underlying(DW_FORM_data16));
  s.append_uleb(2);
  s.append_cstr("a.pep");
  s.append(u8{0});
  s.allocate(16, 0xAA);
  s.append_cstr("b.pep");
  s.append(u8{0});
  s.allocate(16, 0xBB);

  const auto program_start = s.size();
  s.append(bits::span<const u8>{copy_program});
  s.set(offsetof(Packed, unit_length), U32<ElfEndian::be>(s.size() - line_unit_length_end));
  s.set(offsetof(Packed, header_length), U32<ElfEndian::be>(program_start - line_header_length_end));
  const auto bytes = s.get(0, s.size());
  return {bytes.begin(), bytes.end()};
}
} // namespace

TEST_CASE("DWARF line program header", "[scope:dwarf][kind:unit][arch:*]") {
  using Packed = PackedLineProgramHeader<ElfEndian::be>;

  SECTION("Computed lengths") {
    const auto unit = pep10_unit();
    // Section 6.2.4: items 6-11 (6), item 12 (12), items 13-14 (3), item 15 (1), item 16 ("/\0" = 2),
    // items 17-18 (5), item 19 (1), item 20 ("a.pep\0" + ULEB 0 = 7).
    CHECK(unit.header.header_length() == 37);
    // Items 2-5 (8), the header after item 5, and the program.
    CHECK(unit.unit_length() == 8 + 37 + 1);
  }

  SECTION("Write a big-endian unit") {
    // Section 6.2.4, item by item. Multi-byte fixed fields are big-endian.
    const std::vector<u8> expected{
        0x00, 0x00, 0x00, 0x2e,                            // 1: unit_length (46)
        0x00, 0x05,                                        // 2: version
        0x02,                                              // 3: address_size
        0x00,                                              // 4: segment_selector_size
        0x00, 0x00, 0x00, 0x25,                            // 5: header_length (37)
        0x01, 0x01, 0x01, 0xfb, 0x0e, 0x0d,                // 6-11
        0x00, 0x01, 0x01, 0x01, 0x01, 0x00,                // 12: standard_opcode_lengths
        0x00, 0x00, 0x01, 0x00, 0x00, 0x01,                //
        0x01, 0x01, 0x08,                                  // 13-14: DW_LNCT_path, DW_FORM_string
        0x01,                                              // 15: directories_count
        '/', 0x00,                                         // 16
        0x02, 0x01, 0x08, 0x02, 0x0f,                      // 17-18: + DW_LNCT_directory_index, DW_FORM_udata
        0x01,                                              // 19: file_names_count
        'a', '.', 'p', 'e', 'p', 0x00, 0x00,               // 20
        bits::to_underlying(StandardOpcodes::DW_LNS_copy), // program
    };
    CHECK(write_unit<ElfEndian::be>(pep10_unit()) == expected);
  }

  SECTION("Round-trip a little-endian unit") { check_round_trip<ElfEndian::le>(); }
  SECTION("Round-trip a big-endian unit") { check_round_trip<ElfEndian::be>(); }

  SECTION("Parse unrecognized entry formats") {
    const auto bytes = foreign_format_unit();
    const auto unit = parse_line_program_unit<ElfEndian::be>(bytes, 0);
    REQUIRE(unit.has_value());
    CHECK(unit->header.directories == std::vector<std::string>{"/src"});
    REQUIRE(unit->header.files.size() == 2);
    CHECK(unit->header.files[0].path == "a.pep");
    CHECK(unit->header.files[1].path == "b.pep");
    CHECK(unit->header.files[1].directory_index == 0);
    CHECK(std::ranges::equal(unit->program, copy_program));
    CHECK(unit->next_unit == bytes.size());
  }

  SECTION("Units follow one another in a section") {
    const auto unit = write_unit<ElfEndian::be>(pep10_unit());
    auto bytes = unit;
    bytes.insert(bytes.end(), unit.begin(), unit.end());
    const auto first = parse_line_program_unit<ElfEndian::be>(bytes, 0);
    REQUIRE(first.has_value());
    CHECK(first->next_unit == unit.size());
    const auto second = parse_line_program_unit<ElfEndian::be>(bytes, first->next_unit);
    REQUIRE(second.has_value());
    CHECK(second->next_unit == bytes.size());
  }

  SECTION("Reject malformed units") {
    auto bytes = write_unit<ElfEndian::be>(pep10_unit());
    const auto patch = [&](size_t offset, auto value) {
      std::memcpy(bytes.data() + offset, &value, sizeof(value));
    };
    // Section 7.22
    SECTION("Version") { patch(offsetof(Packed, version), U16<ElfEndian::be>(4)); }
    // Section 7.4 item 1
    SECTION("64-bit DWARF") { patch(offsetof(Packed, unit_length), U32<ElfEndian::be>(0xffffffff)); }
    SECTION("unit_length past the section") { bytes.pop_back(); }
    SECTION("header_length past the unit") { patch(offsetof(Packed, header_length), U32<ElfEndian::be>(0xffff)); }
    SECTION("opcode_base of 0") { patch(offsetof(Packed, opcode_base), u8{0}); }
    SECTION("line_range of 0") { patch(offsetof(Packed, line_range), u8{0}); }
    // The directory's path form follows the 12 opcode lengths, format count, and content code.
    SECTION("Unsupported form") {
      patch(sizeof(Packed) + 12 + 2, static_cast<u8>(AttributeForms::DW_FORM_sdata));
    }
    CHECK_FALSE(parse_line_program_unit<ElfEndian::be>(bytes, 0).has_value());
  }
}
