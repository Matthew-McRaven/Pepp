/*
 * Copyright (c) 2024 J. Stanley Warford, Matthew McRaven
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

#include "bts/libs/mapped_file.hpp"
#include <QDebug>
#include <QString>
#include <catch.hpp>
#include <filesystem>
#include <fstream>

#include <string>
#include <vector>

namespace fs = std::filesystem;
using pepp::bts::MappedFile;

// ---------------------
// Helpers
// ---------------------
static fs::path make_temp_path(std::string_view stem) { return fs::path(std::string(stem) + ".bin"); }

static void write_bytes(const fs::path &p, const std::vector<u8> &bytes) {
  std::ofstream os(p, std::ios::binary | std::ios::trunc);
  REQUIRE(os.good());
  os.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  REQUIRE(os.good());
}

static std::vector<u8> read_bytes(const fs::path &p) {
  std::ifstream is(p, std::ios::binary);
  REQUIRE(is.good());
  is.seekg(0, std::ios::end);
  auto n = static_cast<std::size_t>(is.tellg());
  is.seekg(0, std::ios::beg);
  std::vector<u8> out(n);
  is.read(reinterpret_cast<char *>(out.data()), static_cast<std::streamsize>(n));
  REQUIRE(is.good());
  return out;
}

static void ensure_size(const fs::path &p, std::size_t n) {
  // Create file of size n filled with 0 if missing or smaller.
  std::vector<u8> bytes(n, 0);
  write_bytes(p, bytes);
}

TEST_CASE("MappedFile::open_readwrite persists modifications", "[kind:unit][arch:*][scope:bts]") {
  const auto path = make_temp_path("mmap-rw");
  // Ensure file exists and has known initial content.
  std::vector<u8> initial(64);
  std::iota(initial.begin(), initial.end(), 0);
  write_bytes(path, initial);

  { // Force file to be closed before we try to open for reading.
    auto mf = MappedFile::open_readwrite(path.string());
    REQUIRE(mf);

    auto s = mf->slice(8, 16);
    REQUIRE(s);
    REQUIRE(s->size() == 16);

    // Mutate slice
    auto view = s->get();
    REQUIRE(view.size() == 16);
    for (std::size_t i = 0; i < view.size(); ++i) view[i] = static_cast<u8>(0xA0 + i);

    s->flush();
  }

  // Verify by reading directly
  auto bytes = read_bytes(path);
  REQUIRE(bytes.size() == initial.size());
  for (std::size_t i = 0; i < 8; ++i) REQUIRE(bytes[i] == initial[i]);
  for (std::size_t i = 0; i < 16; ++i) REQUIRE(bytes[8 + i] == static_cast<u8>(0xA0 + i));
  for (std::size_t i = 24; i < bytes.size(); ++i) REQUIRE(bytes[i] == initial[i]);

  fs::remove(path);
}

TEST_CASE("MappedFile::open_readonly yields readable spans", "[kind:unit][arch:*][scope:bts]") {
  const auto path = make_temp_path("mmap-ro");

  std::vector<u8> initial(128);
  std::iota(initial.begin(), initial.end(), 0);
  write_bytes(path, initial);
  {
    auto mf = MappedFile::open_readonly(path.string());
    REQUIRE(mf);

    auto s = mf->slice(32, 40);
    REQUIRE(s);
    REQUIRE(s->size() == 40);

    // Force the const path
    const auto &cs = *s;
    auto cview = cs.get(); // span<const u8>
    REQUIRE(cview.size() == 40);
    for (std::size_t i = 0; i < cview.size(); ++i) {
      REQUIRE(cview[i] == initial[32 + i]);
    }
  }
  fs::remove(path);
}

TEST_CASE("MappedFile, multiple slices observe file content consistently", "[kind:unit][arch:*][scope:bts]") {
  const auto path = make_temp_path("mmap-multi");

  std::vector<u8> initial(256);
  std::iota(initial.begin(), initial.end(), 0);
  write_bytes(path, initial);

  {
    auto mf = MappedFile::open_readwrite(path.string());
    REQUIRE(mf);

    auto a = mf->slice(0, 32);
    auto b = mf->slice(16, 32);
    REQUIRE((a && b));

    // Modify overlapping region via slice a
    {
      auto av = a->get();
      REQUIRE(av.size() == 32);
      for (std::size_t i = 16; i < 32; ++i) av[i] = 0xEE; // this overlaps b[0..15]
      a->flush();
    }
  }

  // Read overlap via slice b (may or may not reflect immediately depending on your mapping strategy;
  // but it must reflect after reopening the file).
  auto bytes = read_bytes(path);
  for (std::size_t i = 16; i < 32; ++i) REQUIRE(bytes[i] == 0xEE);

  fs::remove(path);
}

TEST_CASE("MappedFile::open_readwrite can can create && allocate", "[kind:unit][arch:*][scope:bts]") {
  const auto path = make_temp_path("mmap-create");
  // Ensure it doesn't exist.
  fs::remove(path);

  {
    auto mf = MappedFile::open_readwrite(path.string());
    REQUIRE(mf);

    // Ensure the file is at least large enough for our slice.
    // If your open_readwrite does sizing internally, you can remove this.
    ensure_size(path, 64);

    auto s = mf->slice(0, 64);
    REQUIRE(s);
    auto v = s->get();
    REQUIRE(v.size() == 64);

    for (std::size_t i = 0; i < 64; ++i) v[i] = static_cast<u8>(i ^ 0xCC);
    s->flush();
  }

  auto bytes = read_bytes(path);
  REQUIRE(bytes.size() == 64);
  for (std::size_t i = 0; i < 64; ++i) REQUIRE(bytes[i] == static_cast<u8>(i ^ 0xCC));

  fs::remove(path);
}
