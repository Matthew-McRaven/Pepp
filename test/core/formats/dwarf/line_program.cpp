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

#include "core/formats/dwarf/line_program.hpp"
#include <catch.hpp>
#include <ostream>
#include "core/formats/elf/packed_storage.hpp"

using namespace pepp::bts;

namespace {
// A Pep/10-shaped header: 2-byte addresses, one directory, one file.
LineProgramHeader pep10_header() {
  LineProgramHeader header;
  header.address_size = 2;
  header.minimum_instruction_length = 1;
  header.maximum_operations_per_instruction = 1;
  header.default_is_stmt = true;
  header.line_base = -5;
  header.line_range = 14;
  header.standard_opcode_lengths = {0, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 1};
  header.directories = {"/"};
  header.files = {{.path = "a.pep", .directory_index = 0}};
  return header;
}

std::vector<u8> bytes_of(const AStorage &s) {
  const auto bytes = s.get(0, s.size());
  return {bytes.begin(), bytes.end()};
}

struct Row {
  u64 address, file, line;
  bool end_sequence;
  bool operator==(const Row &) const = default;
  // Lets Catch print rows when a comparison fails.
  friend std::ostream &operator<<(std::ostream &os, const Row &row) {
    if (row.end_sequence) return os << std::hex << "0x" << row.address << std::dec << " end";
    return os << std::hex << "0x" << row.address << std::dec << " file " << row.file << " line " << row.line;
  }
};

template <ElfEndian E> std::vector<Row> run(const std::vector<u8> &bytes) {
  const auto unit = parse_line_program_unit<E>(bytes, 0);
  REQUIRE(unit.has_value());
  CHECK(unit->next_unit == bytes.size());
  std::vector<Row> rows;
  const auto ok = run_line_program<E>(*unit, [&](const DebugLineState &s, std::optional<size_t>) {
    // An end_sequence row's other fields are not meaningful.
    if (s.end_sequence) rows.push_back({s.address, 0, 0, true});
    else rows.push_back({s.address, s.file, s.line, false});
  });
  REQUIRE(ok.has_value());
  return rows;
}

template <ElfEndian E> void check_round_trip() {
  BlockStorage s;
  LineProgramWriter<E> writer(s, pep10_header());
  writer.row(0x10, 2, 0, 5);
  // A special opcode alone.
  writer.row(0x12, 0x2e, 0, 6);
  // Too far for a special opcode, even after DW_LNS_const_add_pc. DW_LNS_advance_pc first.
  writer.row(0x40, 1, 0, 3);
  // Too many lines for a special opcode. DW_LNS_advance_line first.
  writer.row(0x41, 0x14, 0, 200);
  // Past a special opcode's reach. DW_LNS_const_add_pc first.
  writer.row(0x55, 1, 0, 200);
  // A lower address must start a new sequence.
  writer.row(0x00, 3, 0, 1);
  CHECK(writer.finish() == 0);
  const std::vector<Row> expected{
      {0x10, 0, 5, false},   {0x12, 0, 6, false}, {0x40, 0, 3, false}, {0x41, 0, 200, false},
      {0x55, 0, 200, false}, {0x56, 0, 0, true},  {0x00, 0, 1, false}, {0x03, 0, 0, true},
  };
  CHECK(run<E>(bytes_of(s)) == expected);
}
} // namespace

TEST_CASE("DWARF line program", "[scope:dwarf][kind:unit][arch:*][!throws]") {
  using enum StandardOpcodes;
  using enum ExtendedOpcodes;
  const auto op = [](auto opcode) { return bits::to_underlying(opcode); };

  SECTION("Write a big-endian program") {
    BlockStorage s;
    const auto header = pep10_header();
    LineProgramWriter<ElfEndian::be> writer(s, header);
    writer.row(0x0000, 3, 0, 1);
    writer.row(0x0003, 1, 0, 2);
    writer.finish();
    // Section 6.2.5.1: opcode = (line increment - line_base) + (line_range * operation advance) + opcode_base.
    const auto special = [&](i64 line_advance, u64 operation_advance) {
      return static_cast<u8>((line_advance - header.line_base) + header.line_range * operation_advance +
                             header.opcode_base());
    };
    // Section 6.2.5. The file register starts at 1 (Table 6.4), but DWARF 5 numbers files from 0.
    const std::vector<u8> expected{
        0x00, 0x03, op(DW_LNE_set_address), 0x00, 0x00, // address 0
        op(DW_LNS_set_file), 0x00,                      // file 0
        special(0, 0),                                  // row
        special(1, 3),                                  // address 3, line 2, row
        op(DW_LNS_advance_pc), 0x01,                    // address 4
        0x00, 0x01, op(DW_LNE_end_sequence),            //
    };
    const auto bytes = bytes_of(s);
    const auto unit = parse_line_program_unit<ElfEndian::be>(bytes, 0);
    REQUIRE(unit.has_value());
    CHECK(unit->next_unit == bytes.size());
    CHECK(std::ranges::equal(unit->program, expected));
  }

  SECTION("Round-trip a little-endian program") { check_round_trip<ElfEndian::le>(); }
  SECTION("Round-trip a big-endian program") { check_round_trip<ElfEndian::be>(); }

  SECTION("A gap between rows ends the sequence") {
    BlockStorage s;
    LineProgramWriter<ElfEndian::be> writer(s, pep10_header());
    writer.row(0x00, 3, 0, 1);
    // Zero-length rows share an address with the next row and so does not end the sequence.
    writer.row(0x03, 0, 0, 2);
    writer.row(0x03, 1, 0, 3);
    // Gap, ends previous sequence.
    writer.row(0x10, 1, 0, 4);
    writer.finish();
    const std::vector<Row> expected{
        {0x00, 0, 1, false}, {0x03, 0, 2, false}, {0x03, 0, 3, false}, {0x04, 0, 0, true},
        {0x10, 0, 4, false}, {0x11, 0, 0, true},
    };
    CHECK(run<ElfEndian::be>(bytes_of(s)) == expected);
  }

  SECTION("Read a hand-written program") {
    const auto header = pep10_header();
    // Section 6.2.5.1: line += line_base + adjusted % line_range, address += adjusted / line_range.
    const u8 special = (1 - header.line_base) + header.line_range * 2 + header.opcode_base();
    const std::vector<u8> program{
        0x00, 0x03, op(DW_LNE_set_address), 0x01, 0x00, // address 0x100
        special,                                        // line 2, address 0x102, row
        op(DW_LNS_const_add_pc),                        // address 0x113
        op(DW_LNS_fixed_advance_pc), 0x00, 0x10,        // address 0x123
        0x00, 0x02, op(DW_LNE_lo_user), 0xaa,           // skipped
        op(DW_LNS_copy),                                // row
        0x00, 0x01, op(DW_LNE_end_sequence),            // row
    };
    BlockStorage s;
    write_line_program_unit<ElfEndian::be>(s, {.header = header, .program = program});
    const std::vector<Row> expected{{0x102, 1, 2, false}, {0x123, 1, 2, false}, {0x123, 0, 0, true}};
    CHECK(run<ElfEndian::be>(bytes_of(s)) == expected);
  }

  SECTION("Reject extended opcodes whose length disagrees with their operands") {
    const auto header = pep10_header();
    std::vector<u8> program;
    // 2-byte addresses make DW_LNE_set_address 3 bytes long, so 4 is one too many.
    SECTION("Too long") { program = {0x00, 0x04, op(DW_LNE_set_address), 0x01, 0x00, 0x00}; }
    SECTION("Too short") { program = {0x00, 0x02, op(DW_LNE_set_address), 0x01}; }
    SECTION("No opcode byte") { program = {0x00, 0x00}; }
    BlockStorage s;
    write_line_program_unit<ElfEndian::be>(s, {.header = header, .program = program});
    const auto bytes = bytes_of(s);
    const auto unit = parse_line_program_unit<ElfEndian::be>(bytes, 0);
    REQUIRE(unit.has_value());
    const auto ignore = [](const DebugLineState &, std::optional<size_t>) {};
    CHECK_FALSE(run_line_program<ElfEndian::be>(*unit, ignore).has_value());
  }

  SECTION("Reject VLIW headers") {
    BlockStorage s;
    auto header = pep10_header();
    header.maximum_operations_per_instruction = 2;
    CHECK_THROWS_AS(LineProgramWriter<ElfEndian::be>(s, header), std::logic_error);
  }

  SECTION("Reject headers without a special opcode for a line increment of 0") {
    BlockStorage s;
    auto header = pep10_header();
    header.line_base = 1;
    CHECK_THROWS_AS(LineProgramWriter<ElfEndian::be>(s, header), std::logic_error);
  }
}
