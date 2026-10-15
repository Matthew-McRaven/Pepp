/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
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

#include <QFile>
#include <catch.hpp>
#include "core/formats/dwarf/line_program.hpp"
#include "core/formats/elf/packed_access_strings.hpp"
#include "core/formats/elf/packed_elf.hpp"

using namespace pepp::bts;

namespace {
struct Row {
  u64 address, file, line;
  bool end_sequence;
  bool operator==(const Row &) const = default;
};
} // namespace

TEST_CASE("DWARF line program from GCC", "[scope:dwarf][kind:unit][arch:riscv]") {
  // Built from data/line_info.c by data/Makefile, which keeps only the line number sections.
  constexpr auto E = ElfEndian::le;
  QFile file(":/dwarf/line_info.elf");
  REQUIRE(file.open(QIODevice::ReadOnly));
  const auto contents = file.readAll();
  PackedInputElfFile<ElfBits::b64, E> elf(
      std::make_shared<BufferSource>(std::vector<char>(contents.cbegin(), contents.cend())));
  const auto section = [&](std::string_view name) -> bits::span<const u8> {
    PackedStringReader<ElfBits::b64, E> names(elf, elf.header.e_shstrndx);
    for (size_t it = 0; it < elf.section_headers.size(); ++it)
      if (const auto n = names.get_string_span(elf.section_headers[it].sh_name); std::string_view(n.data(), n.size()) == name)
        return elf.section_data[it]->get(0, elf.section_data[it]->size());
    return {};
  };
  const auto debug_line = section(".debug_line"), debug_line_str = section(".debug_line_str");
  REQUIRE_FALSE(debug_line.empty());
  REQUIRE_FALSE(debug_line_str.empty());

  // GCC names files with DW_FORM_line_strp, so the paths come from .debug_line_str.
  CHECK_FALSE(parse_line_program_unit<E>(debug_line, 0).has_value());
  const auto unit = parse_line_program_unit<E>(debug_line, 0, debug_line_str);
  REQUIRE(unit.has_value());
  CHECK(unit->next_unit == debug_line.size());
  CHECK(unit->header.address_size == 8);
  CHECK(unit->header.directories == std::vector<std::string>{"."});
  REQUIRE(unit->header.files.size() == 2);
  CHECK(unit->header.files[0].path == "line_info.c");
  CHECK(unit->header.files[1].path == "line_info.c");

  // Matches `riscv64-elf-readelf --debug-dump=decodedline line_info.elf`.
  std::vector<Row> rows;
  const auto ran = run_line_program<E>(*unit, [&](const DebugLineState &s) {
    if (s.end_sequence) rows.push_back({s.address, 0, 0, true});
    else rows.push_back({s.address, s.file, s.line, false});
  });
  REQUIRE(ran.has_value());
  const std::vector<Row> expected{
      {0x100e8, 1, 19, false}, {0x100f6, 1, 19, false}, {0x10100, 1, 19, false}, {0x1010a, 1, 21, false},
      {0x10118, 1, 22, false}, {0x1011c, 1, 23, false}, {0x10122, 1, 23, false}, {0x10124, 1, 23, false},
      {0x10130, 1, 23, false}, {0x1013a, 1, 23, false}, {0x10144, 1, 23, false}, {0x10156, 1, 24, false},
      {0x1015a, 1, 25, false}, {0x10164, 1, 27, false}, {0x1016c, 1, 28, false}, {0x10174, 1, 28, false},
      {0x10178, 1, 29, false}, {0x10190, 1, 30, false}, {0x1019a, 0, 0, true},
  };
  CHECK(rows == expected);
}
