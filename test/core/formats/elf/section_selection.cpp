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

#include <catch.hpp>
#include "core/formats/elf/packed_elf.hpp"
#include "core/formats/elf/packed_io.hpp"
#include "core/formats/elf/packed_ops.hpp"
#include "core/formats/elf/packed_section_selection.hpp"

namespace {
using namespace pepp::bts;
using Selection = PackedSectionSelection<ElfBits::b32, ElfEndian::le>;

// An in-memory object with null, .shstrtab, an allocated .text, and an unallocated .comment.
AnyInputElf object() {
  PackedGrowableElfFile<ElfBits::b32, ElfEndian::le> elf(ElfFileType::ET_REL, ElfMachineType::EM_PEP10,
                                                         ElfABI::ELFOSABI_NONE);
  ensure_section_header_table(elf);
  const auto text = add_named_section(elf, ".text", SectionTypes::SHT_PROGBITS);
  elf.section_headers[text].sh_flags |= bits::to_underlying(SectionFlags::SHF_ALLOC);
  add_named_section(elf, ".comment", SectionTypes::SHT_PROGBITS);
  return to_input_elf(elf);
}

bool allocated(const Selection::Ref &ref) {
  const u64 flags = ref.header.sh_flags;
  return (flags & bits::to_underlying(SectionFlags::SHF_ALLOC)) != 0;
}
} // namespace

TEST_CASE("PackedSectionSelection", "[scope:core][scope:core.formats][kind:unit][arch:*]") {
  PackedInputElfGroupLE32 group;
  const auto a = group.add(object()).value(), b = group.add(object()).value();

  SECTION("Select all sections") {
    // Including each file's null section.
    const Selection all(group);
    CHECK(all.size() == 8);
    CHECK(all.handle(4) == InputElfSectionHandle{b, 0});
  }
  SECTION("Select section subset") {
    const Selection alloc(group, allocated);
    REQUIRE(alloc.size() == 2);
    CHECK(alloc.handle(0) == InputElfSectionHandle{a, 2});
    CHECK(alloc.handle(1) == InputElfSectionHandle{b, 2});
    for (u32 it = 0; it < alloc.size(); ++it) CHECK(alloc.position(alloc.handle(it)) == it);
    REQUIRE(alloc.handles(a).size() == 1);
    CHECK(alloc.handles(a)[0] == InputElfSectionHandle{a, 2});
    CHECK_FALSE(alloc.contains(InputElfSectionHandle{b, 3}));
    CHECK_FALSE(alloc.contains(InputElfSectionHandle{}));
    CHECK_THROWS_AS(alloc.handle(2), std::out_of_range);
    CHECK_THROWS_AS(alloc.handles(ElfFileID{}), std::out_of_range);
  }
  SECTION("Selection does not track later changes to group") {
    const Selection before(group, allocated);
    const auto c = group.add(object()).value();
    CHECK(before.files() == 2);
    CHECK(before.size() == 2);
    CHECK_FALSE(before.contains(InputElfSectionHandle{c, 2}));
    CHECK_THROWS_AS(before.handles(c), std::out_of_range);
    const Selection after(group, allocated);
    REQUIRE(after.size() == 3);
    for (u32 it = 0; it < before.size(); ++it) CHECK(after.handle(it) == before.handle(it));
    CHECK(after.position(InputElfSectionHandle{c, 2}) == 2u);
  }
}
