/*
 * Copyright (c) 2023-2025 J. Stanley Warford, Matthew McRaven
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
#include "bts/elf/elf.hpp"
#include "bts/elf/header.hpp"
namespace {
bool write(const std::string &fname, const std::span<const u8> &data) {
  std::ofstream out(fname, std::ios::binary);
  if (!out.is_open()) return false;
  out.write(reinterpret_cast<const char *>(data.data()), data.size());
  return out.good();
}
} // namespace

TEST_CASE("Test custom ELF library, 32-bit", "[scope:elf][kind:unit][arch:*]") {
  using namespace pepp::bts;
  SECTION("Create ehdr with custom, read with ELFIO") {
    auto my_header = ElfEhdrLE32(FileType::ET_EXEC, MachineType::EM_PEP10, ElfABI::ELFOSABI_NONE);
    auto data = bits::span<const u8>{reinterpret_cast<const u8 *>(&my_header), sizeof(my_header)};
    CHECK(data.size() == 52);
    write("ehdr_only_test32.elf", data);
    ELFIO::elfio elf;
    std::istringstream in(std::string((const char *)data.data(), data.size()));
    CHECK_NOTHROW(elf.load(in) == true);
    CHECK(elf.get_class() == ELFIO::ELFCLASS32);
  }
  SECTION("Create ehdr with section table") {
    ElfLE32 elf(FileType::ET_EXEC, MachineType::EM_PEP8, ElfABI::ELFOSABI_NONE);
    elf.add_section_header_table();
    auto layout = elf.calculate_layout();
    std::vector<u8> data;
    data.resize(size_for_layout(layout));
    elf.write(data, layout);
    write("ehdr_shdr32.elf", data);
    ELFIO::elfio elfio;
    std::istringstream in(std::string((const char *)data.data(), data.size()));
    CHECK_NOTHROW(elfio.load(in) == true);
    CHECK(elfio.get_class() == ELFIO::ELFCLASS32);
    CHECK(elfio.sections.size() == 2);
  }
  SECTION("Create ehdr with section table and program header table") {
    ElfLE32 elf(FileType::ET_EXEC, MachineType::EM_PEP8, ElfABI::ELFOSABI_NONE);
    elf.add_section_header_table();
    ElfPhdrLE32 phdr;
    phdr.p_filesz = 0;
    phdr.p_memsz = 0x1000;
    phdr.p_flags = to_underlying(SegmentFlags::PF_R) | to_underlying(SegmentFlags::PF_W);
    phdr.p_type = to_underlying(SegmentType::PT_LOAD);
    phdr.p_align = 1;
    phdr.p_paddr = phdr.p_vaddr = 0xFEEDBEEF;
    elf.add_segment(std::move(phdr));
    auto layout = elf.calculate_layout();
    std::vector<u8> data;
    data.resize(size_for_layout(layout));
    elf.write(data, layout);
    write("ehdr_all_hdr32.elf", data);
    ELFIO::elfio elfio;
    std::istringstream in(std::string((const char *)data.data(), data.size()));
    CHECK_NOTHROW(elfio.load(in) == true);
    CHECK(elfio.get_class() == ELFIO::ELFCLASS32);
    CHECK(elfio.sections.size() == 2);
    CHECK(elfio.segments.size() == 1);
    CHECK(elfio.segments[0]->get_file_size() == 0);
    CHECK(elfio.segments[0]->get_memory_size() == 4096);
    CHECK(elfio.segments[0]->get_sections_num() == 0);
  }
}

TEST_CASE("Test custom ELF library, 64-bit", "[scope:elf][kind:unit][arch:*]") {
  SECTION("Create ehdr with custom, read with ELFIO") {
    using namespace pepp::bts;
    auto my_header = ElfEhdrLE64(FileType::ET_EXEC, MachineType::EM_RISCV, ElfABI::ELFOSABI_NONE);
    auto data = bits::span<const u8>{reinterpret_cast<const u8 *>(&my_header), sizeof(my_header)};
    CHECK(data.size() == 64);
    write("ehdr_only_test64.elf", data);
    ELFIO::elfio elf;
    std::istringstream in(std::string((const char *)data.data(), data.size()));
    CHECK_NOTHROW(elf.load(in) == true);
    CHECK(elf.get_class() == ELFIO::ELFCLASS64);
  }
}
