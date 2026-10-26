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
#include <initializer_list>
#include <string_view>
#include <vector>
#include "core/formats/elf/packed_io.hpp"

namespace {
using namespace pepp::bts;
using namespace pepp::link;

// An in-memory object with null, .shstrtab, then a PROGBITS section for each name, in order.
AnyInputElf object(std::initializer_list<std::string_view> names) {
  PackedGrowableElfFile<ElfBits::b32, ElfEndian::le> elf(ElfFileType::ET_REL, ElfMachineType::EM_PEP10,
                                                         ElfABI::ELFOSABI_NONE);
  ensure_section_header_table(elf);
  for (const auto name : names) add_named_section(elf, name, SectionTypes::SHT_PROGBITS);
  return to_input_elf(elf);
}
} // namespace

TEST_CASE("SectionPlacements", "[scope:core][scope:core.link][kind:unit][arch:*]") {
  using Handles = std::vector<InputElfSectionHandle>;
  PackedInputElfGroupLE32 group;
  // a is .text = 2, .text.hot = 3, .comment = 4, .text.unlikely = 5; b is .text = 2, .comment = 3.
  const auto a = group.add(object({".text", ".text.hot", ".comment", ".text.unlikely"})).value();
  const auto b = group.add(object({".text", ".comment"})).value();
  SectionPlacements<ElfBits::b32, ElfEndian::le> placements(group);
  Glob hot, comment, text;
  REQUIRE(hot.add(".text.hot", 0));
  REQUIRE(comment.add(".comment", 0));
  REQUIRE(text.add(".text*", 0));

  // Emulate the script `.text : { *(.text.hot) } /DISCARD/ : { *(.comment) } .text : { *(.text*) }`.
  const OutputSectionID text_out{1}, discard_out{2};
  // First text pattern matches only hot. Second text pattern matches any text.
  const InputSectionDescriptionID hot_desc{10}, comment_desc{20}, text_desc{30};
  const auto hot_count = placements.claim(select_unclaimed(group, hot, placements), text_out, hot_desc);
  const auto comment_count = placements.claim(select_unclaimed(group, comment, placements), discard_out, comment_desc);
  const auto text_count = placements.claim(select_by_name(group, text), text_out, text_desc);

  SECTION("Claim respect previously claimed sections") {
    // Only a has a hot
    CHECK(hot_count == 1);
    // Both a and b have comment
    CHECK(comment_count == 2);
    // .text* matches a's .text.hot too, but it was already claimed by hot_desc
    CHECK(text_count == 3);
  }
  SECTION("Input sections record which output section claimed them") {
    CHECK(placements.claim_of(InputElfSectionHandle{a, 3}) == SectionClaim{text_out, hot_desc});
    CHECK(placements.claim_of(InputElfSectionHandle{b, 2}) == SectionClaim{text_out, text_desc});
    CHECK(placements.claim_of(InputElfSectionHandle{b, 3}) == SectionClaim{discard_out, comment_desc});
    // Neither .shstrtab nor the null section were matched.
    CHECK_FALSE(placements.claimed(InputElfSectionHandle{a, 1}));
    CHECK_FALSE(placements.claimed(InputElfSectionHandle{a, 0}));
  }
  SECTION("Sort by description, then by file, then section index") {
    const auto members = placements.members();
    const auto text_members = members.members(text_out);
    // Grouped by description: a:.text.hot (3) before a:.text (2).
    // Then by file: a:.text.unlikely (5) before b:.text (2).
    // Then by section index: a:.text (2) before a:.text.unlikely (5).
    CHECK(Handles(text_members.begin(), text_members.end()) == Handles{{a, 3}, {a, 2}, {a, 5}, {b, 2}});
    const auto discarded = members.members(discard_out);
    CHECK(Handles(discarded.begin(), discarded.end()) == Handles{{a, 4}, {b, 3}});
    CHECK(members.members(OutputSectionID{}).empty());
  }
  SECTION("Invalid claims and lookups") {
    CHECK(placements.claim(select_by_name(group, text), text_out, text_desc) == 0);
    CHECK_THROWS_AS(placements.claim(select_by_name(group, text), OutputSectionID{}, text_desc), std::invalid_argument);
    // Parenthesized, because the macro would split at the handle's comma.
    CHECK_THROWS_AS((placements.claim_of(InputElfSectionHandle{ElfFileID{3}, 0})), std::out_of_range);
  }
}
