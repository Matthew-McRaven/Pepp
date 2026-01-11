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
#include <zpp_bits.h>
#include "bts/elf/architectures.hpp"
#include "bts/elf/elf.hpp"
#include "bts/elf/header.hpp"
namespace {
bool write(const std::string &fname, const std::span<const std::byte> &data) {
  std::ofstream out(fname, std::ios::binary);
  if (!out.is_open()) return false;
  out.write(reinterpret_cast<const char *>(data.data()), data.size());
  return out.good();
}
} // namespace

TEST_CASE("Test custom ELF library, 32-bit", "[scope:elf][kind:unit][arch:*]") {
  using namespace pepp::bts;
  SECTION("Create ehdr with custom, read with ELFIO") {
    auto my_header = ElfEhdr<PEP10>(FileType::ET_EXEC, ElfABI::ELFOSABI_NONE);
    auto [data, _, out] = zpp::bits::data_in_out();
    CHECK(out(my_header).code == std::errc());
    CHECK(data.size() == 52);
    write("ehdr_only_test32.elf", data);
    ELFIO::elfio elf;
    std::istringstream in(std::string((const char *)data.data(), data.size()));
    CHECK_NOTHROW(elf.load(in) == true);
    CHECK(elf.get_class() == ELFIO::ELFCLASS32);
  }
  SECTION("Create ehdr with section table") {
    Elf<PEP10> elf(FileType::ET_EXEC, ElfABI::ELFOSABI_NONE);
    elf.add_section_header_table();
    elf.calculate_layout();
    auto [data, _, out] = zpp::bits::data_in_out();
    CHECK(out(elf).code == std::errc());
    write("ehdr_shdr32.elf", data);
  }
}

TEST_CASE("Test custom ELF library, 64-bit", "[scope:elf][kind:unit][arch:*]") {
  SECTION("Create ehdr with custom, read with ELFIO") {
    using namespace pepp::bts;
    auto my_header = ElfEhdr<RV64LE>(FileType::ET_EXEC, ElfABI::ELFOSABI_NONE);
    auto [data, _, out] = zpp::bits::data_in_out();
    CHECK(out(my_header).code == std::errc());
    CHECK(data.size() == 64);
    write("ehdr_only_test64.elf", data);
    ELFIO::elfio elf;
    std::istringstream in(std::string((const char *)data.data(), data.size()));
    CHECK_NOTHROW(elf.load(in) == true);
    CHECK(elf.get_class() == ELFIO::ELFCLASS64);
  }
}
