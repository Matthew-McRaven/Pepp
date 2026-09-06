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

#pragma once
#include <optional>
#include <string_view>
#include "core/ds/string_pool.hpp"

namespace pepp::bts {

// Represents a SHT_STRTAB section, backed by a StringPool. Uses opaque handles rather than offsets to provide pointer
// stability on insert. Serialized data may not match insertion order due to pooling.
class ManagedStringTable {
public:
  // Reuses an existing entry if possible, otherwise allocates a new entry. The string will be stored with a
  // null-terminator, as required by ELF.
  PooledString insert(std::string_view str);
  std::optional<PooledString> find(std::string_view str) const;
  // If non-empty, returns a pointer to the string, not including its null terminator.
  std::optional<std::string_view> get(PooledString id) const;

  // Minimum number of contiguous bytes to hold this table
  u32 serialized_size() const noexcept;
  // dest must be at least serialized_size() bytes.
  void serialize(bits::span<u8> dest) const;

  // Potentially invalidated on insert(). Exposed for testing, but it is otherwise an implementation
  // detail of serialize.
  u32 offset_of(PooledString id) const;

private:
  // Pre-initializd with offset 0 containing an explict null-terminator.
  StringPool _pool = StringPool::with_null_entry();
};

} // namespace pepp::bts
