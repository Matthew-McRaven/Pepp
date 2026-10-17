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

#include "core/formats/dwarf/macro.hpp"
#include <array>
#include <catch.hpp>

using namespace pepp::bts;

TEST_CASE("DWARF macro information", "[scope:core][scope:core.formats][kind:unit][arch:*]") {
  using enum MacroOpcodes;
  const auto b = [](auto value) { return static_cast<u8>(bits::to_underlying(value)); };
  const auto parse = [](const std::vector<u8> &bytes, size_t offset = 0) {
    return parse_macro_unit<ElfEndian::le>(bits::span<const u8>(bytes.data(), bytes.size()), offset);
  };

  SECTION("Define strings") {
    const std::array<std::string_view, 2> parameters{"a", "b"};
    CHECK(format_define("m", parameters, "LDWA \\a,d\nSTWA \\b,d") == "m(a,b) LDWA \\a,d\nSTWA \\b,d");
    CHECK(format_define("m", {}, "") == "m() ");
    CHECK_THROWS_AS(format_define("a b", {}, ""), std::logic_error);

    const auto function = parse_define("m(a,b) LDWA \\a,d\nSTWA \\b,d");
    REQUIRE(function.has_value());
    CHECK(function->name == "m");
    CHECK(function->parameters == std::vector<std::string_view>{"a", "b"});
    CHECK(function->definition == "LDWA \\a,d\nSTWA \\b,d");
    const auto empty = parse_define("m() ");
    REQUIRE(empty.has_value());
    CHECK(empty->parameters == std::vector<std::string_view>{});
    CHECK(empty->definition.empty());
    // Section 6.3.2.2: an object-like macro has no parameter list.
    const auto object = parse_define("X 1");
    REQUIRE(object.has_value());
    CHECK(object->name == "X");
    CHECK(!object->parameters.has_value());
    CHECK(object->definition == "1");
    CHECK(!parse_define("m(a b").has_value());
  }

  SECTION("Round-trip a unit") {
    BlockStorage s;
    // A unit need not begin the section.
    s.append(u8{0xAA});
    MacroUnitWriter<ElfEndian::le> unit(s, 0x10);
    CHECK(unit.debug_line_offset_field() == 4);
    unit.define(0, "pre() x");
    unit.start_file(0, 1);
    unit.define(3, "m(a) \\a\nnop");
    unit.end_file();
    CHECK(unit.finish() == 1);
    // clang-format off
    const std::vector<u8> expected{
        0xAA,
        // Section 6.3.1 items 1-3: version, flags with only debug_line_offset_flag, debug_line_offset.
        0x05, 0x00, 0x02, 0x10, 0x00, 0x00, 0x00,
        b(DW_MACRO_define), 0x00, 'p', 'r', 'e', '(', ')', ' ', 'x', 0x00,
        b(DW_MACRO_start_file), 0x00, 0x01,
        b(DW_MACRO_define), 0x03, 'm', '(', 'a', ')', ' ', '\\', 'a', '\n', 'n', 'o', 'p', 0x00,
        b(DW_MACRO_end_file),
        0x00,
    };
    // clang-format on
    const auto bytes = s.get(0, s.size());
    REQUIRE(std::vector<u8>(bytes.begin(), bytes.end()) == expected);

    const auto parsed = parse(expected, 1);
    REQUIRE(parsed.has_value());
    CHECK(parsed->debug_line_offset == 0x10);
    CHECK(parsed->entries == std::vector<MacroEntry>{{DW_MACRO_define, 0, "pre() x", false},
                                                     {DW_MACRO_define, 3, "m(a) \\a\nnop", true}});
    CHECK(parsed->next_unit == expected.size());
  }

  SECTION("Opcodes in the opcode_operands_table are skipped") {
    const std::vector<u8> bytes{
        // No debug_line_offset, then a table describing one vendor opcode with a udata and a string operand.
        0x05, 0x00, 0x04, 0x01, b(DW_MACRO_lo_user), 0x02, b(AttributeForms::DW_FORM_udata),
        b(AttributeForms::DW_FORM_string), b(DW_MACRO_lo_user), 0x05, 'h', 'i', 0x00, b(DW_MACRO_define), 0x00, 'x',
        '(', ')', ' ', 0x00, 0x00};
    const auto parsed = parse(bytes);
    REQUIRE(parsed.has_value());
    CHECK(!parsed->debug_line_offset.has_value());
    CHECK(parsed->entries == std::vector<MacroEntry>{{DW_MACRO_define, 0, "x() ", false}});
  }

  SECTION("Malformed units are errors") {
    // An opcode which the table does not describe, a missing terminator, an unsupported version, and an unmatched
    // DW_MACRO_end_file.
    CHECK(!parse({0x05, 0x00, 0x00, b(DW_MACRO_lo_user), 0x00}).has_value());
    CHECK(!parse({0x05, 0x00, 0x00, b(DW_MACRO_define), 0x00, 'x', 0x00}).has_value());
    CHECK(!parse({0x04, 0x00, 0x00, 0x00}).has_value());
    CHECK(!parse({0x05, 0x00, 0x00, b(DW_MACRO_end_file), 0x00}).has_value());
    // Section 6.3.3.1 item 1: DW_MACRO_start_file requires the header to refer to .debug_line.
    BlockStorage s;
    MacroUnitWriter<ElfEndian::le> unit(s, std::nullopt);
    CHECK_THROWS_AS(unit.start_file(0, 1), std::logic_error);
  }
}
