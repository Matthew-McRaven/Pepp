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
#include "core/formats/ar/packed_archive.hpp"
#include "core/formats/elf/packed_elf.hpp"
#include "core/formats/elf/packed_input_group.hpp"
#include "core/formats/elf/packed_io.hpp"
#include "core/formats/elf/packed_ops.hpp"
#include "core/sim/api/memory.hpp"

namespace {
using namespace pepp::bts;

void write_bytes(const std::string &fname, const std::vector<u8> &bytes) {
  std::ofstream(fname, std::ios::binary).write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
}

// An ELF file with one PROGBITS section holding contents, covered by one PT_LOAD segment loaded at base_address.
template <ElfBits B = ElfBits::b32, ElfEndian E = ElfEndian::le>
std::vector<u8> single_section_bytes(std::vector<u8> contents, u64 base_address = 0,
                                     ElfMachineType machine = ElfMachineType::EM_PEP10) {
  PackedGrowableElfFile<B, E> elf(ElfFileType::ET_EXEC, machine, ElfABI::ELFOSABI_NONE);
  ensure_section_header_table(elf);
  const auto idx = add_named_section(elf, ".text", SectionTypes::SHT_PROGBITS);
  elf.section_data[idx]->append(bits::span<const u8>{contents});
  elf.add_segment(SegmentType::PT_LOAD, SegmentFlags::PF_R);
  const std::vector<SegmentLayoutConstraint> constraints{
      {.alignment = 1, .from_sec = idx, .to_sec = idx, .base_address = base_address}};
  return elf_bytes(elf, &constraints);
}

// An ELF file whose first PT_LOAD covers a PROGBITS section followed by a NOBITS one, so memory size is larger than
// filesize. The second PT_LOAD covers only the NOBITS section, so filesize is 0.
std::vector<u8> text_and_bss_bytes(std::vector<u8> text, u32 bss_size, u64 base_address) {
  PackedGrowableElfFile<ElfBits::b32, ElfEndian::le> elf(ElfFileType::ET_EXEC, ElfMachineType::EM_PEP10,
                                                         ElfABI::ELFOSABI_NONE);
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
  return elf_bytes(elf, &constraints);
}

std::vector<u8> bytes_of(const std::shared_ptr<const AStorage> &data) {
  const auto span = data->get(0, data->size());
  return {span.begin(), span.end()};
}

std::shared_ptr<const AStorage> storage_of(const std::vector<u8> &bytes) {
  return std::make_shared<BlockStorage>(std::vector<char>(bytes.begin(), bytes.end()));
}

// Extract only PROGBITS section of a file.
template <class Group> InputElfSectionHandle progbits_of(const Group &group, ElfFileID id) {
  for (const auto handle : group.sections(id))
    if (group.header(handle).sh_type == bits::to_underlying(SectionTypes::SHT_PROGBITS)) return handle;
  FAIL("No PROGBITS section");
  return InputElfSectionHandle{};
}

// Add an ELF file to a group, asserting success, and returning the ID.
template <class Group> ElfFileID add_elf(Group &group, const std::string &path) {
  const auto ret = group.add(path);
  REQUIRE(ret);
  REQUIRE(std::holds_alternative<ElfFileID>(*ret));
  return std::get<ElfFileID>(*ret);
}
} // namespace

TEST_CASE("PackedInputElfGroup", "[scope:core][scope:core.formats][kind:unit][arch:*]") {
  using enum InputGroupError::Code;
  const auto a_bytes = single_section_bytes({0x01, 0x02, 0x03}, 0x100);
  const auto b_bytes = single_section_bytes({0x04, 0x05}, 0x200);
  write_bytes("group_a.elf", a_bytes);
  write_bytes("group_b.elf", b_bytes);
  PackedGrowableArchive regular, thin(true);
  for (auto *archive : {&regular, &thin}) {
    REQUIRE(archive->add_member("group_a.elf", storage_of(a_bytes)));
    REQUIRE(archive->add_member("group_b.elf", storage_of(b_bytes)));
  }
  write_bytes("group.a", archive_bytes(regular).value());
  write_bytes("group_thin.a", archive_bytes(thin).value());
  PackedInputElfGroupLE32 group;
  const auto a = add_elf(group, "group_a.elf"), b = add_elf(group, "group_b.elf");

  SECTION("Section handles transformed into headers + data") {
    CHECK(a != b);
    const auto text = progbits_of(group, a), data = progbits_of(group, b);
    CHECK(group.header(text).sh_size == 3);
    CHECK(bytes_of(group.data(text)) == std::vector<u8>{0x01, 0x02, 0x03});
    CHECK(bytes_of(group.data(data)) == std::vector<u8>{0x04, 0x05});
    // Both files have the same layout, so only the file ID tells these handles apart.
    CHECK(text.index == data.index);
    CHECK(text.file == a);
    CHECK(data.file == b);
    // Handles order by file, then by index.
    CHECK(text < data);
    CHECK(InputElfSectionHandle{a, 0} < text);
  }
  SECTION("Segments") {
    const auto loadable = loadable_segments(group);
    REQUIRE(loadable.size() == 2);
    CHECK(loadable[0].first == group.segments(a)[0]);
    CHECK(loadable[0].second == AddressSpan(0x100, 0x102));
    CHECK(loadable[1].first == group.segments(b)[0]);
    CHECK(loadable[1].second == AddressSpan(0x200, 0x201));
    // Mapped on demand and cached with a shared, overlapping mapping.
    const auto data = group.data(loadable[0].first);
    CHECK(bytes_of(data) == bytes_of(group.data(progbits_of(group, a))));
    CHECK(group.data(loadable[0].first) == data);
    // Zero-filling memory is responsibility of loader.
    write_bytes("group_bss.elf", text_and_bss_bytes({0x06, 0x07}, 4, 0x300));
    const auto segs = group.segments(add_elf(group, "group_bss.elf"));
    REQUIRE(segs.size() == 2);
    CHECK(group.header(segs[0]).p_memsz == 6);
    CHECK(bytes_of(group.data(segs[0])) == std::vector<u8>{0x06, 0x07});
    CHECK(group.data(segs[1])->size() == 0);
  }
  SECTION("Group elements record their source") {
    const auto *path = std::get_if<ElfFromPath>(&group.provenance(a));
    REQUIRE(path != nullptr);
    CHECK(path->path == "group_a.elf");
    const auto in_memory = group.add(open_input_elf("group_b.elf"));
    REQUIRE(in_memory);
    CHECK(std::holds_alternative<ElfInMemory>(group.provenance(*in_memory)));
  }
  SECTION("Extract archive members to group") {
    for (const std::string path : {"group.a", "group_thin.a"}) {
      const auto before = group.size();
      const auto added = group.add(path);
      REQUIRE(added);
      REQUIRE(std::holds_alternative<ElfArchiveID>(*added));
      const auto id = std::get<ElfArchiveID>(*added);
      CHECK(group.archive(id).path() == std::filesystem::path(path));
      // An archive by itself contributes no files.
      CHECK(group.size() == before);
      std::vector<std::vector<u8>> contents;
      for (const auto &member : group.archive(id).members()) {
        REQUIRE(member);
        const auto file = group.add_member(id, *member);
        REQUIRE(file);
        const auto *from = std::get_if<ElfFromArchive>(&group.provenance(*file));
        REQUIRE(from != nullptr);
        CHECK(from->archive == id);
        CHECK(from->member.header_offset == member->header_offset);
        contents.push_back(bytes_of(group.data(progbits_of(group, *file))));
      }
      CHECK(contents == std::vector<std::vector<u8>>{{0x01, 0x02, 0x03}, {0x04, 0x05}});
    }
  }
  SECTION("Rejected inputs do not modify group") {
    std::ofstream("group_junk.bin", std::ios::binary) << "Gibberish not matching any magic";
    // Different endianess, then machine.
    write_bytes("group_be.elf", single_section_bytes<ElfBits::b32, ElfEndian::be>({0x01}));
    write_bytes("group_rv.elf", single_section_bytes({0x01}, 0, ElfMachineType::EM_RISCV));
    auto code_of = [&](const std::string &path) {
      const auto ret = group.add(path);
      REQUIRE_FALSE(ret);
      return ret.error().code;
    };
    CHECK(code_of("group_missing.elf") == CannotOpen);
    CHECK(code_of("group_junk.bin") == UnknownFormat);
    CHECK(code_of("group_be.elf") == WrongClassOrByteOrder);
    CHECK(code_of("group_rv.elf") == WrongMachine);
    CHECK(group.add_member(ElfArchiveID{}, PackedArMember{}).error().code == NoSuchArchive);
    // Thin archive with non-existent members.
    auto archive = to_input_archive(thin);
    REQUIRE(archive);
    const auto member = *archive->members().begin();
    REQUIRE(member);
    const auto id = group.add(std::move(*archive));
    REQUIRE(id);
    const auto file = group.add_member(*id, *member);
    REQUIRE_FALSE(file);
    CHECK(file.error().code == BadArchive);
    CHECK(file.error().archive->code == ArchiveError::Code::ThinMember);
    CHECK(group.size() == 2);
  }
  SECTION("Validate out-of-range handles") {
    CHECK_THROWS_AS(group.file(ElfFileID{}), std::out_of_range);
    CHECK_THROWS_AS(group.file(ElfFileID{3}), std::out_of_range);
    const auto count = static_cast<u32>(group.sections(a).size());
    CHECK_THROWS_AS(group.header(InputElfSectionHandle{a, count}), std::out_of_range);
    CHECK_THROWS_AS(group.header(InputElfSegmentHandle{a, 1}), std::out_of_range);
    CHECK_THROWS_AS(group.data(InputElfSegmentHandle{a, 1}), std::out_of_range);
    CHECK_THROWS_AS(group.header(InputElfSectionHandle{}), std::out_of_range);
    CHECK_FALSE(InputElfSectionHandle{});
    // A packed 16-bit index would have wrapped this to the null section.
    CHECK_THROWS_AS(group.header(InputElfSectionHandle{a, 0x1'0000}), std::out_of_range);
    CHECK_THROWS_AS(group.data(InputElfSectionHandle{a, 0x1'0000}), std::out_of_range);
  }
}
