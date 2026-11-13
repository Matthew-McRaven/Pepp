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

#include "core/link/section_placements.hpp"
#include <catch.hpp>
#include <filesystem>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>
#include "./helpers.hpp"
#include "core/formats/elf/packed_io.hpp"
#include "core/langs/asmb/diagnostic_table.hpp"
#include "core/langs/ldscript/parser.hpp"

namespace {
using namespace pepp::bts;
using namespace pepp::link;
using Elf = PackedGrowableElfFile<ElfBits::b32, ElfEndian::le>;

// An object with null, .shstrtab, then a PROGBITS section for each name, in order.
Elf object(std::initializer_list<std::string_view> names) {
  Elf elf(ElfFileType::ET_REL, ElfMachineType::EM_PEP10, ElfABI::ELFOSABI_NONE);
  ensure_section_header_table(elf);
  for (const auto name : names) add_named_section(elf, name, SectionTypes::SHT_PROGBITS);
  return elf;
}

// Add an object that is held in memory, and so has no name for a file pattern to match.
ElfFileID add_in_memory(PackedInputElfGroupLE32 &group, std::initializer_list<std::string_view> names) {
  auto elf = object(names);
  return group.add(to_input_elf(elf)).value();
}

// Write an object to path, then add it to group.
ElfFileID add_from_path(PackedInputElfGroupLE32 &group, const std::string &path,
                        std::initializer_list<std::string_view> names) {
  auto elf = object(names);
  link_test::write_bytes(path, elf_bytes(elf));
  return std::get<ElfFileID>(group.add(std::filesystem::path(path)).value());
}
} // namespace

TEST_CASE("place_sections", "[scope:core][scope:core.link][kind:unit][arch:*]") {
  using Handles = std::vector<InputElfSectionHandle>;
  PackedInputElfGroupLE32 group;
  // Section indices start at 2, after the null section and .shstrtab.
  const auto a = add_from_path(group, "place_a.o", {".text", ".text.hot", ".comment", ".text.unlikely", ".data"});
  const auto b = add_from_path(group, "place_b.o", {".text", ".comment", ".rodata"});
  link_test::write_bytes("place_lib.a", link_test::archive_bytes_of({{{"x", true}}}));
  const auto lib = std::get<ElfArchiveID>(group.add(std::filesystem::path("place_lib.a")).value());
  const auto first_member = *group.archive(lib).members().begin();
  REQUIRE(first_member);
  const auto member = group.add_member(lib, *first_member).value();
  const auto memory = add_in_memory(group, {".text.hot"});
  const auto excluded = add_in_memory(group, {".text"});
  std::vector<Liveness> files(group.size() + 1, Liveness::Live);
  files[excluded.value] = Liveness::Excluded;

  // A plain file name matches a file that is not in an archive, :name only such a file, and archive: any of the
  // archive's members.
  pepp::tc::DiagnosticTable diag;
  const auto script = pepp::tc::ldscript::parse(diag, R"(SECTIONS {
  .text : { *(.text.hot) place_a.o(.text*) }
  .text : { :place_b.o(.text) place_lib.a:(.text) }
  /DISCARD/ : { *(.comment) }
  .data 0x1000 : { *(.data .rodata) }
})");
  REQUIRE(diag.count() == 0);
  auto [outputs, placements] = place_sections(group, files, script);

  // A repeated name creates a unique output section since the and an address is evaluated later.
  REQUIRE(outputs.size() == 5);
  CHECK(outputs[1].name == ".text");
  CHECK(outputs[2].name == ".text");
  CHECK(outputs[4].name == ".data");
  CHECK(!outputs[1].address);
  CHECK(outputs[4].address);

  const auto assigned = [](u32 output, u32 description) {
    return SectionAssignment{OutputSectionID{output}, InputSectionDescriptionID{description}};
  };
  // place_a.o(.text*) also matches a's .text.hot, but *(.text.hot) matched it first.
  CHECK(placements.assignment_of({a, 3}) == assigned(1, 1));
  CHECK(placements.assignment_of({memory, 2}) == assigned(1, 1));
  CHECK(placements.assignment_of({a, 2}) == assigned(1, 2));
  CHECK(placements.assignment_of({b, 2}) == assigned(2, 3));
  CHECK(placements.assignment_of({member, 2}) == assigned(2, 4));
  CHECK(placements.assignment_of({b, 3}) == assigned(3, 5));
  CHECK(placements.assignment_of({b, 4}) == assigned(4, 6));
  // Only live files' sections are placeable. null sections and string table are never assigned.
  CHECK_FALSE(placements.sections().contains({excluded, 2}));
  CHECK_FALSE(placements.sections().contains({a, 0}));
  CHECK_FALSE(placements.sections().contains({a, 1}));

  // Members are sorted by description, then by file, then by section index.
  const auto members = placements.members();
  const auto text = members.members(OutputSectionID{1});
  CHECK(Handles(text.begin(), text.end()) == Handles{{a, 3}, {memory, 2}, {a, 2}, {a, 5}});
  CHECK(members.members(OutputSectionID{}).empty());

  CHECK_THROWS_AS(placements.assign({a, 2}, OutputSectionID{}, InputSectionDescriptionID{2}), std::invalid_argument);
  CHECK_THROWS_AS((placements.assignment_of({excluded, 2})), std::out_of_range);
}
