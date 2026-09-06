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

#include "core/formats/elf/managed_access_strings.hpp"
#include <cstring>
#include <stdexcept>

pepp::bts::PooledString pepp::bts::ManagedStringTable::insert(std::string_view str) {
  return _pool.insert_null_terminated(str);
}

std::optional<pepp::bts::PooledString> pepp::bts::ManagedStringTable::find(std::string_view str) const {
  return _pool.find(NullTerminated{str});
}

std::optional<std::string_view> pepp::bts::ManagedStringTable::get(PooledString id) const {
  // A pooled string's length covers its whole storage, including null-terminator.
  auto str = _pool.find(id);
  if (!str.has_value()) return std::nullopt;
  // Per header comment, drop null terminator if present (and it should be!)
  if (!str->empty() && str->back() == '\0') str->remove_suffix(1);
  return str;
}

u32 pepp::bts::ManagedStringTable::offset_of(PooledString id) const {
  return static_cast<u32>(_pool.byte_offset(id));
}

u32 pepp::bts::ManagedStringTable::serialized_size() const noexcept {
  return static_cast<u32>(_pool.pooled_byte_size());
}

void pepp::bts::ManagedStringTable::serialize(bits::span<u8> dest) const {
  // Pages are laid out contiguously, which makes byte_offset() a valid index into `dest`.
  size_t written = 0;
  for (auto page = _pool.pages_cbegin(); page != _pool.pages_cend(); ++page) {
    const size_t used = page->used_capacity();
    if (used == 0) continue;
    if (written + used > dest.size()) throw std::out_of_range("ManagedStringTable::serialize destination too small");
    std::memcpy(dest.data() + written, page->data(), used);
    written += used;
  }
}
