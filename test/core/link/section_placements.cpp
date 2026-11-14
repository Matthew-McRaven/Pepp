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

#include "core/link/section_layout.hpp"
#include "core/link/section_placements.hpp"
#include <algorithm>
#include <catch.hpp>
#include <filesystem>
#include <initializer_list>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <tuple>
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

// Returns the name of each output section and assigned symbol in a SECTIONS command, in order.
std::vector<std::string> layout_of(const pepp::tc::ldscript::Sections &sections) {
  std::vector<std::string> ret;
  for (const auto &command : sections.commands)
    if (const auto *output = std::get_if<pepp::tc::ldscript::OutputSectionDescription>(&command))
      ret.push_back(output->name);
    else ret.push_back(std::get<pepp::tc::ldscript::Assignment>(command).symbol);
  return ret;
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

// Returns the members of an output section, in order.
std::vector<InputElfSectionHandle> members_of(const SectionPlacements &placements, OutputSectionID output) {
  std::vector<InputElfSectionHandle> ret;
  const auto members = placements.members();
  for (const auto position : members.members(output)) ret.push_back(placements.sections().handle(position));
  return ret;
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
  auto script = pepp::tc::ldscript::parse(diag, R"(SECTIONS {
  .text : { *(.text.hot) place_a.o(.text*) }
  .text : { :place_b.o(.text) place_lib.a:(.text) }
  /DISCARD/ : { *(.comment) }
  .data 0x1000 : { *(.data .rodata) }
})");
  REQUIRE(diag.count() == 0);
  auto &sections = pepp::tc::ldscript::merge_sections(script);
  auto placements = place_sections(group, files, sections);

  // Descriptions are numbered in script order, and a repeated name gets its own output section.
  const auto output = [&](u32 index) -> const auto & {
    return std::get<pepp::tc::ldscript::OutputSectionDescription>(sections.commands[index]);
  };
  CHECK(output(1).id == OutputSectionID{2});
  CHECK(output(3).id == OutputSectionID{4});
  CHECK(std::get<pepp::tc::ldscript::InputSectionDescription>(output(3).commands[0]).id ==
        InputSectionDescriptionID{6});

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
  CHECK(members_of(placements, OutputSectionID{1}) == Handles{{a, 3}, {memory, 2}, {a, 2}, {a, 5}});
  CHECK(members_of(placements, OutputSectionID{}).empty());

  CHECK_THROWS_AS(placements.assign({a, 2}, OutputSectionID{}, InputSectionDescriptionID{2}), std::invalid_argument);
  CHECK_THROWS_AS((placements.assignment_of({excluded, 2})), std::out_of_range);
}

TEST_CASE("orphan sections", "[scope:core][scope:core.link][kind:unit][arch:*]") {
  using namespace bits;
  using enum SectionFlags;
  using enum SectionTypes;
  PackedInputElfGroupLE32 group;
  // Section indices start at 2, after the null section and .shstrtab.
  auto first = link_test::elf_with({{".text", SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR},
                                    {".mycode", SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR},
                                    {".rodata", SHT_PROGBITS, SHF_ALLOC},
                                    {".data", SHT_PROGBITS, SHF_ALLOC | SHF_WRITE},
                                    {".bss", SHT_NOBITS, SHF_ALLOC | SHF_WRITE},
                                    {".mydata", SHT_PROGBITS, SHF_ALLOC | SHF_WRITE},
                                    {".zz_awx", SHT_PROGBITS, SHF_ALLOC | SHF_WRITE | SHF_EXECINSTR},
                                    {".debug_info", SHT_PROGBITS},
                                    {".mybss", SHT_NOBITS, SHF_ALLOC | SHF_WRITE}});
  auto second = link_test::elf_with(
      {{".mycode", SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR}, {".b_ro", SHT_PROGBITS, SHF_ALLOC}});
  const auto a = group.add(to_input_elf(first)).value();
  const auto b = group.add(to_input_elf(second)).value();
  const std::vector<Liveness> files(group.size() + 1, Liveness::Live);
  // Parse text, merge its SECTIONS commands, then assign input sections to outputs.
  const auto placed_by = [&](std::string_view text) {
    pepp::tc::DiagnosticTable diag;
    auto script = pepp::tc::ldscript::parse(diag, text);
    REQUIRE(diag.count() == 0);
    auto sections = std::move(pepp::tc::ldscript::merge_sections(script));
    auto placements = place_sections(group, files, sections);
    return std::pair{std::move(sections), std::move(placements)};
  };

  SECTION("with output sections") {
    auto [sections, placements] = placed_by(R"(SECTIONS { .text 0x1000 : { *(.text) } }
_etext = .;
SECTIONS {
  . = 0x2000;
  .data : { *(.data) }
  .mydata : { *(.none) }
})");
    CHECK(layout_of(sections) == std::vector<std::string>{".text", "_etext", ".mycode", ".rodata", ".b_ro", ".",
                                                          ".data", ".zz_awx", ".bss", ".mybss", ".mydata",
                                                          ".debug_info"});
    // Orphans with the same name share an output section, and an orphan joins an empty output section with its name.
    CHECK(members_of(placements, OutputSectionID{4}) == std::vector<InputElfSectionHandle>{{a, 3}, {b, 2}});
    CHECK(placements.assignment_of({a, 7}) == SectionAssignment{OutputSectionID{3}, InputSectionDescriptionID{7}});
    // The empty .mydata takes its flags and type from the orphan that joined it.
    const auto named = [&](std::string_view name) {
      return &std::get<pepp::tc::ldscript::OutputSectionDescription>(
          sections.commands[*find_output_section(sections.commands, name)]);
    };
    CHECK(named(".text")->flags == to_underlying(SHF_ALLOC | SHF_EXECINSTR));
    CHECK(named(".mydata")->flags == to_underlying(SHF_ALLOC | SHF_WRITE));
    CHECK(named(".mydata")->type == SHT_PROGBITS);
    CHECK(named(".bss")->type == SHT_NOBITS);
    // Only the output sections and input section descriptions added for orphans are marked.
    CHECK(named(".mycode")->orphan);
    CHECK_FALSE(named(".mydata")->orphan);
    CHECK_FALSE(std::get<pepp::tc::ldscript::InputSectionDescription>(named(".mydata")->commands[0]).orphan);
    CHECK(std::get<pepp::tc::ldscript::InputSectionDescription>(named(".mydata")->commands[1]).orphan);
  }

  // Without output sections, every section is an orphan, placed after the first location counter assignment.
  SECTION("without output sections") {
    auto [sections, placements] = placed_by("SECTIONS { . = 0x3000; }");
    CHECK(layout_of(sections) == std::vector<std::string>{".", ".text", ".mycode", ".rodata", ".b_ro", ".data",
                                                          ".mydata", ".zz_awx", ".bss", ".mybss", ".debug_info"});
  }
  SECTION("without SECTIONS") {
    auto [sections, placements] = placed_by("");
    CHECK(layout_of(sections).front() == ".text");
  }
}

TEST_CASE("section layout", "[scope:core][scope:core.link][kind:unit][arch:*]") {
  using enum SectionFlags;
  using enum SectionTypes;
  using namespace bits;
  PackedInputElfGroupLE32 group;
  auto first = link_test::elf_with({
      {".text", SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR, 8, 1},
      {".data", SHT_PROGBITS, SHF_ALLOC | SHF_WRITE, 4, 1},
      {".bss", SHT_NOBITS, SHF_ALLOC | SHF_WRITE, 0, 1},
      {".odd", SHT_PROGBITS, SHF_ALLOC, 1, 4},
  });
  auto second = link_test::elf_with({
      {".text", SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR, 8, 2},
  });
  const auto a = group.add(to_input_elf(first)).value();
  const auto b = group.add(to_input_elf(second)).value();
  std::vector<Liveness> files(group.size() + 1, Liveness::Live);

  pepp::tc::DiagnosticTable diag;
  auto script = pepp::tc::ldscript::parse(diag, R"(SECTIONS {
  .text 0x100 : { *(.text) }
  . = ALIGN(0x10);
  .data : { *(.data) . = ALIGN(8); }
  .empty : { *(.none) }
  .bss : { *(.bss) }
  .back : { . = 0x10; . = 0; }
  .odd 0x232 : { *(.odd) }
})");
  REQUIRE(diag.count() == 0);
  auto &sections = pepp::tc::ldscript::merge_sections(script);
  auto placements = place_sections(group, files, sections);
  Diagnostics diagnostics;
  lay_out_sections(diagnostics, group, placements, sections);
  using Memory = std::tuple<u64, u64, u64, bool>;
  const auto outputs = output_sections(sections);
  const auto memory_of = [&](u32 id) {
    const auto &output = *outputs.at(id);
    return Memory{output.vma, output.size, output.alignment, output.empty};
  };

  // .text holds members with alignments 1 and 2, and ALIGN(8) pads .data to 8 bytes. .empty has no members and .bss
  // has only an empty one, so both are left out. Moving the location counter backwards within .back is an error.
  CHECK(memory_of(1) == Memory{0x100, 0x10, 2, false});
  CHECK(memory_of(2) == Memory{0x110, 8, 1, false});
  CHECK(outputs.at(3)->empty);
  CHECK(outputs.at(4)->empty);
  CHECK(memory_of(5) == Memory{0x118, 0x10, 1, false});
  // An explicit address is kept even when it is less aligned than the members, and the alignment is lowered to match.
  // The member is still aligned to 4 within the section, which pads it by 2 bytes.
  CHECK(memory_of(6) == Memory{0x232, 3, 2, false});
  CHECK(placements.assignment_of({a, 2}).offset == 0);
  CHECK(placements.assignment_of({b, 2}).offset == 8);
  const auto entries = link_test::entries_of(diagnostics);
  REQUIRE(entries.size() == 1);
  CHECK(entries[0].code == DiagnosticCode{LinkCode::LocationCounterBackwards});
  CHECK(std::get<ScriptMessage>(entries[0].subject).message ==
        "cannot move location counter backwards (from 0x128 to 0x118)");
}
