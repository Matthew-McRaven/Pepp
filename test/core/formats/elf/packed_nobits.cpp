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
#include <catch.hpp>
#include <elfio/elfio.hpp>
#include <sstream>
#include <vector>
#include "core/formats/elf/packed_ops.hpp"
#include "core/formats/elf/packed_types.hpp"

/*
 * sh_size is usually the file size, EXCEPT when the section is SHT_NOBITS. In that case, it's memory size.
 * Earlier versions of this library were buggy for NOBITS and set their memory size to 0. These test cases exist to
 * prevent a regression. p_filesz is also impacted by this bugfix.
 */
TEST_CASE("Sections of type SHT_NOBIT occupy no file bytes", "[scope:elf][kind:unit][arch:*][!throws]") {
  using namespace pepp::bts;
  using Packed = PackedGrowableElfLE32;
  constexpr auto b32 = ElfBits::b32;
  constexpr auto le = ElfEndian::le;

  // Four bytes of .text, 0x100 bytes of zero-ed .bss bytesm and a non-loadable .comment section with data.
  auto build = [&](Packed &elf) {
    ensure_section_header_table(elf);
    auto text = add_named_section(elf, ".text", SectionTypes::SHT_PROGBITS);
    elf.section_headers[text].sh_flags = bits::to_underlying(SectionFlags::SHF_ALLOC);
    const u8 code[] = {1, 2, 3, 4};
    elf.section_data[text]->append(bits::span<const u8>{code, 4});

    auto bss = add_named_section(elf, ".bss", SectionTypes::SHT_NOBITS);
    elf.section_headers[bss].sh_flags = bits::to_underlying(SectionFlags::SHF_ALLOC);
    elf.section_headers[bss].sh_size = 0x100;

    auto comment = add_named_section(elf, ".comment", SectionTypes::SHT_PROGBITS);
    const u8 note[] = {0xAB, 0xCD};
    elf.section_data[comment]->append(bits::span<const u8>{note, 2});
    return std::tuple{text, bss, comment};
  };

  SECTION("sh_size survives calculate_layout for NOBITS") {
    Packed elf(ElfFileType::ET_EXEC, ElfMachineType::EM_PEP10, ElfABI::ELFOSABI_NONE);
    auto [text, bss, comment] = build(elf);
    auto layout = calculate_layout(elf);

    CHECK(elf.section_headers[bss].sh_size == 0x100);
    CHECK(elf.section_headers[text].sh_size == 4);
    // A NOBITS section occupies no file, so the next section's offset is the same as the NOBITS section's offset.
    CHECK(elf.section_headers[comment].sh_offset == elf.section_headers[text].sh_offset + 4);
    CHECK(elf.section_headers[bss].sh_offset == elf.section_headers[comment].sh_offset);
  }

  SECTION("Segments ending in NOBITS work as expected") {
    using namespace bits;
    Packed elf(ElfFileType::ET_EXEC, ElfMachineType::EM_PEP10, ElfABI::ELFOSABI_NONE);
    [[maybe_unused]] auto [text, bss, comment] = build(elf);
    elf.add_segment(SegmentType::PT_LOAD, SegmentFlags::PF_R | SegmentFlags::PF_W);

    SegmentLayoutConstraint load;
    load.alignment = 4096;
    load.from_sec = text;
    load.to_sec = bss;
    load.base_address = 0x1000;
    std::vector<SegmentLayoutConstraint> constraints = {load};
    auto layout = calculate_layout(elf, &constraints);

    const auto &phdr = elf.program_headers[0];
    CHECK(phdr.p_filesz == 4);
    // p_memsz has to cover the .bss, or the zero-fill it implies never happens.
    CHECK(phdr.p_memsz >= 4 + 0x100);
    CHECK(phdr.p_offset == elf.section_headers[text].sh_offset);

    //  Validate against a 3rd-party ELF reader to ensure we are producing valid/compliant binaries.
    std::vector<u8> data(size_for_layout(layout), 0);
    write(data, layout);
    ELFIO::elfio reader;
    std::istringstream in(std::string(reinterpret_cast<const char *>(data.data()), data.size()));
    REQUIRE(reader.load(in));

    auto *loaded_bss = reader.sections[".bss"];
    REQUIRE(loaded_bss != nullptr);
    CHECK(loaded_bss->get_type() == ELFIO::SHT_NOBITS);
    CHECK(loaded_bss->get_size() == 0x100);
    CHECK(reader.sections[".text"]->get_size() == 4);

    REQUIRE(reader.segments.size() == 1);
    const auto *loaded_seg = reader.segments[0];
    CHECK(loaded_seg->get_file_size() == 4);
    CHECK(loaded_seg->get_memory_size() >= 4 + 0x100);
    // The whole segment has to lie within the file it claims to come from.
    CHECK(loaded_seg->get_offset() + loaded_seg->get_file_size() <= data.size());
  }

  SECTION("A segment may not store file data after a NOBITS section") {
    using namespace bits;
    Packed elf(ElfFileType::ET_EXEC, ElfMachineType::EM_PEP10, ElfABI::ELFOSABI_NONE);
    [[maybe_unused]] auto [text, bss, comment] = build(elf);
    elf.add_segment(SegmentType::PT_LOAD, SegmentFlags::PF_R | SegmentFlags::PF_W);
    SegmentLayoutConstraint load;
    load.alignment = 4096;
    load.from_sec = text;
    load.to_sec = comment;
    load.base_address = 0x1000;
    std::vector<SegmentLayoutConstraint> constraints = {load};
    CHECK_THROWS_AS(calculate_layout(elf, &constraints), std::logic_error);
  }

  SECTION("A segment may end with multiple NOBITS sections") {
    using namespace bits;
    Packed elf(ElfFileType::ET_EXEC, ElfMachineType::EM_PEP10, ElfABI::ELFOSABI_NONE);
    ensure_section_header_table(elf);
    auto text = add_named_section(elf, ".text", SectionTypes::SHT_PROGBITS);
    const u8 code[] = {1, 2, 3, 4};
    elf.section_data[text]->append(bits::span<const u8>{code, 4});
    auto bss = add_named_section(elf, ".bss", SectionTypes::SHT_NOBITS);
    elf.section_headers[bss].sh_size = 0x100;
    auto tbss = add_named_section(elf, ".tbss", SectionTypes::SHT_NOBITS);
    elf.section_headers[tbss].sh_size = 0x10;
    elf.add_segment(SegmentType::PT_LOAD, SegmentFlags::PF_R | SegmentFlags::PF_W);

    SegmentLayoutConstraint load;
    load.alignment = 4096;
    load.from_sec = text;
    load.to_sec = tbss;
    load.base_address = 0x1000;
    std::vector<SegmentLayoutConstraint> constraints = {load};
    REQUIRE_NOTHROW(calculate_layout(elf, &constraints));
    CHECK(elf.program_headers[0].p_filesz == 4);
    CHECK(elf.program_headers[0].p_memsz >= 4 + 0x100 + 0x10);
  }
}
