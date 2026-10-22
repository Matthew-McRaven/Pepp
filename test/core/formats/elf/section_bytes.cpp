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

#include <algorithm>
#include <array>
#include <catch.hpp>
#include <vector>
#include "core/formats/elf/packed_elf.hpp"
#include "core/formats/elf/packed_ops.hpp"

TEST_CASE("Section bytes by offset and by address", "[scope:elf][kind:unit][arch:*]") {
  using namespace pepp::bts;
  PackedGrowableElfLE32 elf(ElfFileType::ET_EXEC, ElfMachineType::EM_RISCV, ElfABI::ELFOSABI_NONE);
  ensure_section_header_table(elf);
  const auto text = add_named_section(elf, ".text", SectionTypes::SHT_PROGBITS);
  const auto bss = add_named_section(elf, ".bss", SectionTypes::SHT_NOBITS);
  const auto comment = add_named_section(elf, ".comment", SectionTypes::SHT_PROGBITS);
  const std::array<u8, 4> bytes{1, 2, 3, 4};
  elf.section_data[text]->append(bits::span<const u8>(bytes));
  elf.section_data[comment]->append(bits::span<const u8>(bytes));
  for (const auto &[index, address, size] : {std::tuple<u16, u32, u32>{text, 0x100, 4}, {bss, 0x104, 8}}) {
    auto &header = elf.section_headers[index];
    header.sh_flags = bits::to_underlying(SectionFlags::SHF_ALLOC);
    header.sh_addr = address, header.sh_size = size;
  }
  elf.section_headers[comment].sh_size = 4;

  SECTION("By address") {
    CHECK(section_at_address(elf, 0x100) == text);
    CHECK(section_at_address(elf, 0x103) == text);
    CHECK(section_at_address(elf, 0x104) == bss);
    CHECK(section_at_address(elf, 0x10C) == 0);
    // .comment is at address 0, but is not SHF_ALLOC.
    CHECK(section_at_address(elf, 0) == 0);
  }
  SECTION("By offset") {
    const auto middle = section_bytes(elf, text, 1, 2);
    REQUIRE(middle.has_value());
    CHECK(std::ranges::equal(*middle, std::array<u8, 2>{2, 3}));
    CHECK(section_bytes(elf, text, 4, 0).has_value());
    CHECK_FALSE(section_bytes(elf, text, 3, 2).has_value());
    CHECK_FALSE(section_bytes(elf, text, 5, 0).has_value());
    // NOBITS sections have no data.
    CHECK_FALSE(section_bytes(elf, bss, 0, 1).has_value());
  }
  SECTION("By offset in paged storage") {
    // String tables use PagedAllocator, so ensure that section_bytes works on the contained strings.
    const auto strings = add_named_section(elf, ".strings", SectionTypes::SHT_STRTAB);
    auto &data = *elf.section_data[strings];
    const std::vector<u8> first(4000, 0xAA), second(200, 0xBB);
    data.append(bits::span<const u8>(first));
    const auto at = data.append(bits::span<const u8>(second));
    CHECK(section_bytes(elf, strings).empty());
    const auto within = section_bytes(elf, strings, at, second.size());
    REQUIRE(within.has_value());
    CHECK(std::ranges::equal(*within, second));
    CHECK_FALSE(section_bytes(elf, strings, at - 1, 2).has_value());
  }
}
