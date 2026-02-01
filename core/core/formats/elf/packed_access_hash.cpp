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

#include "core/formats/elf/packed_access_hash.hpp"
// As specified by: https://refspecs.linuxfoundation.org/elf/gabi4+/ch5.dynamic.html#hash
u32 pepp::bts::elf_hash(bits::span<const char> name) noexcept {
  u32 h = 0, g;
  for (const auto &c : name) {
    h = (h << 4) + c;
    if ((g = h & 0xf0000000)) h ^= g >> 24;
    h &= ~g;
  }
  return h;
}

u32 pepp::bts::elf_hash(std::string_view name) noexcept { return elf_hash(bits::span<const char>{name}); }

// As specified by: https://blogs.oracle.com/solaris/gnu-hash-elf-sections-v2
u32 pepp::bts::gnu_elf_hash(bits::span<const char> name) noexcept {
  u32 h = 5381;
  for (const auto &c : name) {
    h = (h << 5) + h + c;
  }
  return h;
}

u32 pepp::bts::gnu_elf_hash(std::string_view name) noexcept { return gnu_elf_hash(bits::span<const char>{name}); }
