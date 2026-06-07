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
#include <span>
#include "core/ds/hash/djb.hpp"
#include "core/ds/hash/elf_hash.hpp"
TEST_CASE("Quick validations for hash functions", "[scope:elf][kind:unit][arch:*]") {
  using namespace pepp;
  SECTION("ELF hash") {
    // Sample hashes from: https://flapenguin.me/elf-dt-hash
    CHECK(elf_hash(std::span{"", 0}) == 0);
    CHECK(elf_hash(std::span{"printf", 6}) == 0x077905a6);
    CHECK(elf_hash(std::span{"exit", 4}) == 0x0006cf04);
    CHECK(elf_hash(std::span{"syscall", 7}) == 0x0b09985c);
    CHECK(elf_hash(std::span{"flapenguin.me", 13}) == 0x03987915);
  }
  SECTION("djb") {
    // Do not include null terminators for sake of matching existing hashes
    CHECK((u32)djb(std::span{"", 0}) == 0x00001505);
    CHECK((u32)djb(std::span{"printf", 6}) == 0x156b2bb8);
    CHECK((u32)djb(std::span{"exit", 4}) == 0x7c967e3f);
    CHECK((u32)djb(std::span{"syscall", 7}) == 0xbac212a0);
    CHECK((u32)djb(std::span{"flapenguin.me", 13}) == 0x8ae9f18e);
  }
}