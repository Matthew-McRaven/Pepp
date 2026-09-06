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

#include "core/formats/elf/managed_to_packed.hpp"
#include <algorithm>
#include <catch.hpp>
#include <stdexcept>
#include <vector>
#include "core/formats/elf/managed_access_strings.hpp"
#include "core/formats/elf/managed_elf.hpp"
#include "core/formats/elf/managed_section.hpp"
#include "core/formats/elf/managed_segment.hpp"

TEST_CASE("Garbage collect sections", "[kind:unit][arch:*][!throws][tc2][scope:elf]") {
  using namespace pepp::bts;
  ManagedElf elf(ElfBits::b32, ElfEndian::be, ElfFileType::ET_EXEC, ElfMachineType::EM_PEP10);

  SECTION("Pseudo-sections are dropped") {
    auto text = elf.add_section(".text", SectionTypes::SHT_PROGBITS);
    auto live = garbage_collect_sections(elf);
    REQUIRE(live.size() == 1);
    CHECK(live[0] == text);
  }

  SECTION("Predicates work as expected") {
    auto keep = elf.add_section(".keep", SectionTypes::SHT_PROGBITS);
    elf.add_section(".drop", SectionTypes::SHT_PROGBITS);
    auto live = garbage_collect_sections(elf, [](const ManagedSection &sec) { return sec.name == ".keep"; });
    CHECK(live == std::vector<SectionRef>{keep});
  }

  SECTION("Transitive deps are kept (sh_link)") {
    auto strtab = elf.add_section(".strtab", SectionTypes::SHT_STRTAB);
    auto symtab = elf.add_section(".symtab", SectionTypes::SHT_SYMTAB);
    auto rel = elf.add_section(".rel.text", SectionTypes::SHT_REL);
    elf.section(symtab)->link = strtab;
    elf.section(rel)->link = symtab;
    auto live = garbage_collect_sections(elf, [](const ManagedSection &sec) { return sec.name == ".rel.text"; });
    CHECK(live == std::vector<SectionRef>{strtab, symtab, rel});
  }

  SECTION("Transitive deps are kept (sh_info)") {
    auto text = elf.add_section(".text", SectionTypes::SHT_PROGBITS);
    auto rel = elf.add_section(".rel.text", SectionTypes::SHT_REL);
    elf.section(rel)->info = text;
    auto by_ref = garbage_collect_sections(elf, [](const ManagedSection &sec) { return sec.name == ".rel.text"; });
    CHECK(by_ref == std::vector<SectionRef>{text, rel});
    // Plain integer values are not treated as section refs.
    elf.section(rel)->info = u32{1};
    auto by_count = garbage_collect_sections(elf, [](const ManagedSection &sec) { return sec.name == ".rel.text"; });
    CHECK(by_count == std::vector<SectionRef>{rel});
  }

  SECTION("Pseudo-sections cannot be resurected") {
    auto text = elf.add_section(".text", SectionTypes::SHT_PROGBITS);
    elf.section(text)->link = ManagedElf::SHN_ABS;
    elf.section(text)->info = ManagedElf::SHN_COMMON;
    CHECK(garbage_collect_sections(elf) == std::vector<SectionRef>{text});
  }

  SECTION("Reference cycles do not crash") {
    auto a = elf.add_section(".a", SectionTypes::SHT_PROGBITS);
    auto b = elf.add_section(".b", SectionTypes::SHT_PROGBITS);
    elf.section(a)->link = b;
    elf.section(b)->link = a;
    auto live = garbage_collect_sections(elf, [](const ManagedSection &sec) { return sec.name == ".a"; });
    CHECK(live == std::vector<SectionRef>{a, b});
  }
}

