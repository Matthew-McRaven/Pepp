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

#include "core/formats/elf/packed_io.hpp"
#include <catch.hpp>
#include <fstream>
#include <iterator>
#include "core/formats/elf/packed_input_group.hpp"
#include "core/formats/elf/packed_ops.hpp"

namespace {
using namespace pepp::bts;

std::vector<u8> bytes_of(const std::shared_ptr<const AStorage> &data) {
  const auto span = data->get(0, data->size());
  return {span.begin(), span.end()};
}

std::vector<u8> file_bytes(const std::string &path) {
  std::ifstream in(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
} // namespace

TEST_CASE("Serialize a growable ELF", "[scope:elf][kind:unit][arch:*]") {
  const std::vector<u8> contents{0x01, 0x02, 0x03};
  PackedGrowableElfLE32 elf(ElfFileType::ET_EXEC, ElfMachineType::EM_PEP10, ElfABI::ELFOSABI_NONE);
  ensure_section_header_table(elf);
  const auto text = add_named_section(elf, ".text", SectionTypes::SHT_PROGBITS);
  elf.section_data[text]->append(bits::span<const u8>{contents});
  elf.add_segment(SegmentType::PT_LOAD, SegmentFlags::PF_R);
  const std::vector<SegmentLayoutConstraint> constraints{
      {.alignment = 1, .from_sec = text, .to_sec = text, .base_address = 0x100}};

  // All serialization paths should produce the same bytes.
  const auto expected = elf_bytes(elf, &constraints);

  // The file is re-parsed from serialized bytes, so it knows nothing of the growable file it came from.
  const auto check_contents = [&](const auto &in) {
    CHECK(in.header.e_machine == bits::to_underlying(ElfMachineType::EM_PEP10));
    CHECK(in.section_headers.size() == elf.section_headers.size());
    REQUIRE(in.program_headers.size() == 1);
    CHECK(in.program_headers[0].p_vaddr == 0x100);
    CHECK(in.program_headers[0].p_filesz == contents.size());
    CHECK(bytes_of(in.section_data[text]) == contents);
    CHECK(bytes_of(in.segment_data(0)) == contents);
  };

  SECTION("Private memory-mapped region") {
    const auto in = to_input_elf(elf, &constraints);
    REQUIRE(in != nullptr);
    check_contents(*in);
  }
  SECTION("Memory-mapped file") {
    const std::string path = "packed_io_mapped.elf";
    // A longer file from a previous run must not leave a tail behind.
    const std::string junk(expected.size() + 4096, '\xFF');
    std::ofstream(path, std::ios::binary).write(junk.data(), junk.size());
    const auto in = to_input_elf(elf, &constraints, path);
    REQUIRE(in != nullptr);
    check_contents(*in);
    CHECK(file_bytes(path) == expected);
  }
  SECTION("Result can be used in an InputElfGroup") {
    auto any = to_input_group(to_input_elf(elf, &constraints));
    const auto &group = *std::get<std::unique_ptr<PackedInputElfGroupLE32>>(any);
    REQUIRE(group.size() == 1);
    const auto loadable = loadable_segments(group);
    REQUIRE(loadable.size() == 1);
    CHECK(loadable[0].second == pepp::core::Interval<u32>(0x100, 0x102));
    CHECK(bytes_of(group.data(loadable[0].first)) == contents);
  }
}
