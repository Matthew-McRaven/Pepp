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
#include <fstream>
#include "core/formats/elf/packed_elf.hpp"
#include "core/formats/elf/packed_input_group.hpp"
#include "core/formats/elf/packed_ops.hpp"
#include "core/sim/api/memory.hpp"

namespace {
using namespace pepp::bts;

// Write an ELF file with one PROGBITS section holding contents, covered by one PT_LOAD segment loaded at base_address.
template <ElfBits B, ElfEndian E>
void write_single_section(const std::string &fname, std::string_view name, std::vector<u8> contents,
                          u64 base_address = 0) {
  PackedGrowableElfFile<B, E> elf(ElfFileType::ET_EXEC, ElfMachineType::EM_PEP10, ElfABI::ELFOSABI_NONE);
  ensure_section_header_table(elf);
  const auto idx = add_named_section(elf, name, SectionTypes::SHT_PROGBITS);
  elf.section_data[idx]->append(bits::span<const u8>{contents});
  elf.add_segment(SegmentType::PT_LOAD, SegmentFlags::PF_R);
  const std::vector<SegmentLayoutConstraint> constraints{
      {.alignment = 1, .from_sec = idx, .to_sec = idx, .base_address = base_address}};
  auto layout = calculate_layout(elf, &constraints);
  std::vector<u8> bytes(size_for_layout(layout), 0);
  write(bytes, layout);
  std::ofstream(fname, std::ios::binary).write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
}

// Write an ELF file whose first PT_LOAD covers a PROGBITS section followed by a NOBITS one, so its memory size
// exceeds its file size, and whose second PT_LOAD covers the NOBITS section alone, so it has no file bytes at all.
template <ElfBits B, ElfEndian E>
void write_text_and_bss(const std::string &fname, std::vector<u8> text, u64 bss_size, u64 base_address) {
  PackedGrowableElfFile<B, E> elf(ElfFileType::ET_EXEC, ElfMachineType::EM_PEP10, ElfABI::ELFOSABI_NONE);
  ensure_section_header_table(elf);
  const auto text_idx = add_named_section(elf, ".text", SectionTypes::SHT_PROGBITS);
  elf.section_data[text_idx]->append(bits::span<const u8>{text});
  const auto bss_idx = add_named_section(elf, ".bss", SectionTypes::SHT_NOBITS);
  // NOBITS occupies no file space, so its size is the caller's to declare.
  elf.section_headers[bss_idx].sh_size = bss_size;
  elf.add_segment(SegmentType::PT_LOAD, SegmentFlags::PF_R);
  elf.add_segment(SegmentType::PT_LOAD, SegmentFlags::PF_W);
  const std::vector<SegmentLayoutConstraint> constraints{
      {.alignment = 1, .from_sec = text_idx, .to_sec = bss_idx, .base_address = base_address},
      {.alignment = 1, .from_sec = bss_idx, .to_sec = bss_idx, .base_address = base_address}};
  auto layout = calculate_layout(elf, &constraints);
  std::vector<u8> bytes(size_for_layout(layout), 0);
  write(bytes, layout);
  std::ofstream(fname, std::ios::binary).write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
}

std::vector<u8> bytes_of(const std::shared_ptr<const AStorage> &data) {
  const auto span = data->get(0, data->size());
  return {span.begin(), span.end()};
}
} // namespace

TEST_CASE("PackedInputElfGroup", "[scope:elf][kind:unit][arch:*]") {
  write_single_section<ElfBits::b32, ElfEndian::le>("group_a.elf", ".text", {0x01, 0x02, 0x03}, 0x100);
  write_single_section<ElfBits::b32, ElfEndian::le>("group_b.elf", ".data", {0x04, 0x05}, 0x200);
  PackedInputElfGroupLE32 group;
  const auto a = group.open("group_a.elf"), b = group.open("group_b.elf");

  // The only PROGBITS section in a file written by write_single_section.
  auto progbits = [&](ElfFileID id) {
    for (const auto handle : group.sections(id))
      if (group.header(handle).sh_type == bits::to_underlying(SectionTypes::SHT_PROGBITS)) return handle;
    FAIL("No PROGBITS section");
    return ElfSectionHandle{};
  };

  SECTION("Files get distinct IDs") {
    CHECK(group.size() == 2);
    CHECK(a);
    CHECK(b);
    CHECK(a != b);
  }
  SECTION("Section handles transformed into headers + data") {
    const auto text = progbits(a), data = progbits(b);
    CHECK(group.header(text).sh_size == 3);
    CHECK(bytes_of(group.data(text)) == std::vector<u8>{0x01, 0x02, 0x03});
    CHECK(group.header(data).sh_size == 2);
    CHECK(bytes_of(group.data(data)) == std::vector<u8>{0x04, 0x05});
    // Both files have the same layout, so only the file ID tells these handles apart.
    CHECK(elf_index_of(text) == elf_index_of(data));
    CHECK(elf_file_of(text) == a);
    CHECK(elf_file_of(data) == b);
    CHECK(text != data);
  }
  SECTION("Translate segment handles to their headers") {
    const auto segs = group.segments(a);
    REQUIRE(segs.size() == 1);
    CHECK(group.header(segs[0]).p_type == bits::to_underlying(SegmentType::PT_LOAD));
    CHECK(group.header(segs[0]).p_filesz == 3);
  }
  SECTION("Extract loadable segments from a group") {
    const auto loadable = loadable_segments(group);
    REQUIRE(loadable.size() == 2);
    CHECK(loadable[0].first == group.segments(a)[0]);
    CHECK(loadable[0].second == AddressSpan(0x100, 0x102));
    CHECK(loadable[1].first == group.segments(b)[0]);
    CHECK(loadable[1].second == AddressSpan(0x200, 0x201));
  }
  SECTION("Segment data is mapped on demand and cached") {
    const auto &file = group.file(a);
    const auto first = file.segment_data(0);
    REQUIRE(first != nullptr);
    CHECK(bytes_of(first) == std::vector<u8>{0x01, 0x02, 0x03});
    // The same file region reached through the section, i.e. a second mapping overlapping the first.
    CHECK(bytes_of(group.data(progbits(a))) == bytes_of(first));
    CHECK(file.segment_data(0) == first);
  }
  SECTION("Segment data stops at p_filesz") {
    write_text_and_bss<ElfBits::b32, ElfEndian::le>("group_bss.elf", {0x06, 0x07}, 4, 0x300);
    const auto c = group.open("group_bss.elf");
    const auto segs = group.segments(c);
    REQUIRE(segs.size() == 2);
    CHECK(group.header(segs[0]).p_filesz == 2);
    CHECK(group.header(segs[0]).p_memsz == 6);
    // The zero-filled tail out to p_memsz is the loader's business, not the storage's.
    CHECK(bytes_of(group.file(c).segment_data(0)) == std::vector<u8>{0x06, 0x07});
    CHECK(group.header(segs[1]).p_filesz == 0);
    CHECK(group.file(c).segment_data(1)->size() == 0);
  }
  SECTION("Validate out-of-range handles") {
    CHECK_THROWS_AS(group.file(a).segment_data(1), std::out_of_range);
    CHECK_THROWS_AS(group.file(ElfFileID{}), std::out_of_range);
    CHECK_THROWS_AS(group.file(ElfFileID{3}), std::out_of_range);
    const auto count = static_cast<u16>(group.sections(a).size());
    CHECK_THROWS_AS(group.header(make_elf_handle<ElfSectionHandle>(a, count)), std::out_of_range);
    CHECK_THROWS_AS(group.header(make_elf_handle<ElfSegmentHandle>(a, 1)), std::out_of_range);
    CHECK_THROWS_AS(group.header(ElfSectionHandle{}), std::out_of_range);
  }
  SECTION("Disallow opening a file with a different endianness or size") {
    write_single_section<ElfBits::b32, ElfEndian::be>("group_be.elf", ".text", {0x01});
    CHECK_THROWS_AS(group.open("group_be.elf"), std::runtime_error);
    CHECK(group.size() == 2);
  }
}
