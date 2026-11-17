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
#include <filesystem>
#include <fstream>
#include <iterator>
#include <numeric>
#include <string>
#include <utility>
#include <vector>
#include "core/io/mapped_file.hpp"

namespace {
using pepp::bts::BufferedFile;

void write_file(const std::string &path, const std::vector<u8> &bytes) {
  std::ofstream(path, std::ios::binary)
      .write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

std::vector<u8> read_file(const std::string &path) {
  std::ifstream file(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(file), {}};
}

std::vector<u8> bytes_of(bits::span<const u8> span) { return {span.begin(), span.end()}; }
} // namespace

TEST_CASE("BufferedFile", "[scope:core][scope:core.io][kind:unit][arch:*][!throws]") {
  std::vector<u8> initial(16);
  std::iota(initial.begin(), initial.end(), u8(0));

  SECTION("Overlapping slices share data") {
    write_file("buffered_rw.bin", initial);
    auto file = BufferedFile::open_readwrite("buffered_rw.bin");
    auto a = file->slice(0, 8), b = file->slice(4, 8);
    a->get()[5] = 0xEE;
    CHECK(b->get()[1] == 0xEE);
    // Offset 11 is outside of a, so flushing a must not write it.
    b->get()[7] = 0xDD;
    a->flush();
    auto expected = initial;
    expected[5] = 0xEE;
    CHECK(read_file("buffered_rw.bin") == expected);
    b->flush();
    expected[11] = 0xDD;
    CHECK(read_file("buffered_rw.bin") == expected);
  }

  SECTION("Read-only slices do not extend file") {
    write_file("buffered_ro.bin", initial);
    const auto file = BufferedFile::open_readonly("buffered_ro.bin");
    CHECK(file->size() == 16);
    const auto slice = std::as_const(*file).slice(12, 8);
    CHECK(slice->readonly());
    CHECK(slice->size() == 8);
    CHECK(bytes_of(slice->get()) == std::vector<u8>{12, 13, 14, 15});
    std::filesystem::remove("buffered_missing.bin");
    CHECK_THROWS(BufferedFile::open_readonly("buffered_missing.bin"));
  }

  SECTION("Truncating & resizing zero-fill") {
    write_file("buffered_new.bin", initial);
    auto truncated = BufferedFile::open_truncated("buffered_new.bin", 4);
    CHECK(read_file("buffered_new.bin") == std::vector<u8>(4));
    const auto slice = truncated->slice(0, 4);
    slice->get()[3] = 9;
    slice->flush();
    CHECK(read_file("buffered_new.bin") == std::vector<u8>{0, 0, 0, 9});
    auto grown = BufferedFile::open_readwrite("buffered_new.bin", 6);
    CHECK(bytes_of(grown->slice(0, 6)->get()) == std::vector<u8>{0, 0, 0, 9, 0, 0});
    CHECK(read_file("buffered_new.bin").size() == 6);
  }
}
