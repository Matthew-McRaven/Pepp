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
#include <cstring>
#include <string_view>
#include <type_traits>
#include "../../integers.h"
#include "core/math/bitmanip/span.hpp"

namespace pepp::bts {

// Sequential reader over DWARF bytes. A failed read yields zeros / empty values and latches an `error`.
// Callers can check error state once after reading a group of values rather than after each field.
struct DwarfCursor {
  bits::span<const u8> data;
  size_t offset = 0;
  const char *error = nullptr;

  bool ok() const noexcept { return error == nullptr; }
  size_t remaining() const noexcept { return offset < data.size() ? data.size() - offset : 0; }
  // Latch an error (keeping the first one) and stop all further reads.
  void fail(const char *why) noexcept;

  bits::span<const u8> bytes(size_t length) noexcept;
  // Packed, fixed-size values such as U32<E> or a packed header. Endianness is handled by T.
  template <typename T> T fixed() noexcept {
    static_assert(std::is_trivially_copyable_v<T>, "Requires trivially copyable types");
    static_assert(alignof(T) == 1, "Types must be packed / have alignment of 1");
    T ret{};
    if (auto span = bytes(sizeof(T)); span.size() == sizeof(T)) std::memcpy(&ret, span.data(), sizeof(T));
    return ret;
  }
  u64 uleb() noexcept;
  i64 sleb() noexcept;
  // NUL-terminated string. The view excludes the terminator.
  std::string_view cstr() noexcept;
};

} // namespace pepp::bts
