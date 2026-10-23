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
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include "core/formats/elf/packed_elf.hpp"
#include "core/formats/elf/packed_storage.hpp"
#include "core/io/mapped_file.hpp"

namespace {
using namespace pepp::bts;

const std::string contents = "0123456789";

std::shared_ptr<MappedFile> write_file(const std::string &path) {
  std::ofstream(path, std::ios::binary).write(contents.data(), contents.size());
  return MappedFile::open_readonly(path);
}

std::string text_of(const std::shared_ptr<AStorage> &data) {
  const auto span = data->get(0, data->size());
  return {span.begin(), span.end()};
}
} // namespace

TEST_CASE("ELF sources overflow checks", "[scope:core][scope:core.formats][kind:unit][arch:*]") {
  SECTION("MappedFile") { CHECK(write_file("elf_source_file.bin")->size() == contents.size()); }

  SECTION("BufferSource") {
    const BufferSource source(std::vector<char>(contents.begin(), contents.end()));
    CHECK(source.size() == contents.size());
    CHECK(text_of(source.slice(8, 4)) == "89");
    CHECK(source.slice(contents.size() + 1, 4)->size() == 0);
  }

  SECTION("MappedFileSource") {
    const MappedFileSource source(write_file("elf_source_mapped.bin"));
    CHECK(source.size() == contents.size());
    // Slices reaching past end of file are clipped rather than mapped.
    CHECK(text_of(source.slice(8, 4)) == "89");
    CHECK(source.slice(contents.size(), 4)->size() == 0);
    CHECK(source.slice(contents.size() + 4096, 4)->size() == 0);
  }

  SECTION("SliceSource") {
    auto parent = std::make_shared<BufferSource>(std::vector<char>(contents.begin(), contents.end()));
    const SliceSource source(parent, 2, 5);
    CHECK(source.size() == 5);
    CHECK(text_of(source.slice(0, 5)) == "23456");
    // Clipped to the window, not to the parent.
    CHECK(text_of(source.slice(3, 4)) == "56");
    CHECK(source.slice(5, 1)->size() == 0);

    CHECK_NOTHROW(SliceSource(parent, 0, contents.size()));
    CHECK_NOTHROW(SliceSource(parent, contents.size(), 0));
    CHECK_THROWS_AS(SliceSource(parent, 2, contents.size() - 1), std::invalid_argument);
    CHECK_THROWS_AS(SliceSource(parent, contents.size() + 1, 0), std::invalid_argument);
    CHECK_THROWS_AS(SliceSource(nullptr, 0, 0), std::invalid_argument);
  }
}

TEST_CASE("MemoryMapped storage rejects OOB access", "[scope:core][scope:core.formats][kind:unit][arch:*]") {
  auto file = write_file("elf_source_memory_mapped.bin");
  const MemoryMapped storage(file->slice(0, contents.size()));
  const auto size = contents.size();
  REQUIRE(storage.size() == size);

  CHECK(storage.get(size - 2, 2).size() == 2);
  CHECK(storage.get(size, 0).empty());
  CHECK(storage.get(size - 2, 4).empty());
  CHECK(storage.get(size + 1, 0).empty());
  CHECK(storage.get(1, static_cast<size_t>(-1)).empty());

  // No NUL in the file, so a string runs to the end of the slice.
  CHECK(storage.strlen(size - 3) == 3);
  CHECK(storage.strlen(size) == 0);
  CHECK(storage.strlen(size + 100) == 0);
}
