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

#include "core/formats/elf/managed_section_shstrtab.hpp"
#include <catch.hpp>
#include <vector>
#include "core/formats/elf/managed_elf.hpp"
#include "core/formats/elf/managed_section.hpp"
#include "core/formats/elf/managed_section_gc.hpp"
#include "core/formats/elf/managed_section_strtab.hpp"

TEST_CASE("Section header string table creation", "[kind:unit][arch:*][!throws][tc2][scope:elf]") {
  using namespace pepp::bts;
  ManagedElf elf(ElfBits::b32, ElfEndian::be, ElfFileType::ET_EXEC, ElfMachineType::EM_PEP10);

  SECTION("Creation of section expands live list") {
    auto text = elf.add_section(".text", SectionTypes::SHT_PROGBITS);
    CHECK_FALSE(elf.shstrtab());
    auto live = garbage_collect_sections(elf);
    auto shstrtab = build_shstrtab(elf, live);
    CHECK(elf.shstrtab() == shstrtab);
    REQUIRE(elf.section(shstrtab) != nullptr);
    CHECK(elf.section(shstrtab)->name == ".shstrtab");
    CHECK(elf.section(shstrtab)->type == SectionTypes::SHT_STRTAB);
    // Created after the live set was taken, so it has to be added to it.
    CHECK(live == std::vector<SectionRef>{text, shstrtab});
  }

  SECTION("Includes all section names (including itself)") {
    [[maybe_unused]] auto text = elf.add_section(".text", SectionTypes::SHT_PROGBITS);
    [[maybe_unused]] auto data = elf.add_section(".data", SectionTypes::SHT_PROGBITS);
    auto live = garbage_collect_sections(elf);
    auto shstrtab = build_shstrtab(elf, live);
    const auto &table = *elf.section(shstrtab)->content_as<ManagedStringTable>();
    CHECK(table.find(".text").has_value());
    CHECK(table.find(".data").has_value());
    CHECK(table.find(".shstrtab").has_value());
    CHECK_FALSE(table.find("SHN_ABS").has_value());
  }

  SECTION("Existing shstrabs are re-used") {
    auto existing = elf.add_section(".shstrtab", SectionTypes::SHT_STRTAB);
    elf.set_shstrtab(existing);
    elf.add_section(".text", SectionTypes::SHT_PROGBITS);
    auto live = garbage_collect_sections(elf);
    const auto before = live.size();
    auto shstrtab = build_shstrtab(elf, live);
    CHECK(shstrtab == existing);
    CHECK(live.size() == before);
  }
}
