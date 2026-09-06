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
#include "core/formats/elf/managed_elf.hpp"
#include <catch.hpp>
#include <string>
#include <vector>
#include "core/formats/elf/managed_section.hpp"
#include "core/formats/elf/managed_segment.hpp"

TEST_CASE("ManagedElf sanity tests", "[kind:unit][arch:*][!throws][tc2][scope:elf]") {
  using namespace pepp::bts;
  ManagedElf elf(ElfBits::b32, ElfEndian::be, ElfFileType::ET_EXEC, ElfMachineType::EM_PEP10);

  SECTION("Pseudo-sections exist and do not serialize") {
    auto *abs = elf.section(ManagedElf::SHN_ABS);
    auto *common = elf.section(ManagedElf::SHN_COMMON);
    REQUIRE(abs != nullptr);
    REQUIRE(common != nullptr);
    CHECK_FALSE(abs->is_serialized());
    CHECK_FALSE(common->is_serialized());
    // SHN_UNDEF is the null handle, so it names no section of its own.
    CHECK(elf.section(ManagedElf::SHN_UNDEF) == nullptr);
  }

  SECTION("String table sections can be grown incrementally") {
    auto ref = elf.add_section(".strtab", SectionTypes::SHT_STRTAB);
    auto *sec = elf.section(ref);
    REQUIRE(sec != nullptr);
    CHECK(sec->name == ".strtab");
    CHECK(sec->type == SectionTypes::SHT_STRTAB);
    CHECK(sec->is_serialized());

    auto &table = sec->content.emplace<ManagedStringTable>();
    auto main_cln = table.insert("mainCln");
    auto cln = table.insert("Cln");
    auto main = table.insert("main");
    CHECK(table.insert("mainCln") == main_cln);

    // Offset 0 is the empty string, a tail is shared, and a prefix is not.
    CHECK(table.offset_of(cln) == table.offset_of(main_cln) + 4);
    CHECK(table.offset_of(main) != table.offset_of(main_cln));
    CHECK(*table.get(main) == "main");

    // Serialize and re-read the same way consumers would, which is by searching for nul terminators.
    std::vector<u8> bytes(table.serialized_size());
    table.serialize(bits::span<u8>{bytes.data(), bytes.size()});
    auto at = [&](u32 offset) { return std::string(reinterpret_cast<const char *>(bytes.data()) + offset); };
    CHECK(at(0).empty());
    CHECK(at(table.offset_of(main_cln)) == "mainCln");
    CHECK(at(table.offset_of(cln)) == "Cln");
    CHECK(at(table.offset_of(main)) == "main");
  }

  SECTION("SectionRef is stable across inserts") {
    auto strtab = elf.add_section(".strtab", SectionTypes::SHT_STRTAB);
    auto *before = elf.section(strtab);
    before->content.emplace<ManagedStringTable>().insert("main");
    for (int it = 0; it < 32; it++) elf.add_section(".filler" + std::to_string(it), SectionTypes::SHT_PROGBITS);
    CHECK(elf.section(strtab) == before);
    CHECK(std::get<ManagedStringTable>(before->content).find("main").has_value());
  }
}

TEST_CASE("ManagedElf segments", "[kind:unit][arch:*][!throws][tc2][scope:elf]") {
  using namespace pepp::bts;
  ManagedElf elf(ElfBits::b32, ElfEndian::be, ElfFileType::ET_EXEC, ElfMachineType::EM_PEP10);
  auto load = elf.add_segment(SegmentType::PT_LOAD, SegmentFlags::PF_R);

  auto text = elf.add_section(".text", SectionTypes::SHT_PROGBITS);
  elf.section(text)->content.emplace<RawBytes>().bytes = {1, 2, 3, 4};
  auto bss = elf.add_section(".bss", SectionTypes::SHT_NOBITS);
  elf.section(bss)->content.emplace<NoBits>().size = 0x100;

  SECTION("Segments can be constructed") {
    auto *seg = elf.segment(load);
    REQUIRE(seg != nullptr);
    CHECK(seg->type == SegmentType::PT_LOAD);
    CHECK(seg->flags == SegmentFlags::PF_R);
    CHECK(seg->sections.empty());
  }

  SECTION("Segments respect insertion order of sections") {
    auto *seg = elf.segment(load);
    seg->sections.push_back(text);
    seg->sections.push_back(bss);
    REQUIRE(seg->sections.size() == 2);
    CHECK(seg->sections[0] == text);
    CHECK(seg->sections[1] == bss);
    seg->sections.erase(seg->sections.begin());
    REQUIRE(seg->sections.size() == 1);
    CHECK(seg->sections[0] == bss);
  }

  SECTION("Default constucted SegmentRef maps to nullptr") { CHECK(elf.segment(SegmentRef{}) == nullptr); }

  SECTION("SegmentRefs are stable across insertion") {
    auto second = elf.add_segment(SegmentType::PT_LOAD, SegmentFlags::PF_W);
    auto *before = elf.segment(load);
    before->sections.push_back(text);
    for (int it = 0; it < 16; it++) elf.add_segment(SegmentType::PT_NOTE);
    CHECK(elf.segment(load) == before);
    REQUIRE(elf.segment(load)->sections.size() == 1);
    std::size_t seen = 0;
    for (auto it = SegmentRef{1}; it <= elf.last_segment(); ++it) seen++;
    CHECK(seen == 18);
    CHECK(second.value == 2);
  }
}
