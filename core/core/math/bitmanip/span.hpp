/*
 * Copyright (c) 2023-2026 J. Stanley Warford, Matthew McRaven
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
#include <cstdint>
#include <span>
#include <type_traits>
// A header from many years ago.
// At one point, we had to provide our own span, because some supported platforms did not have a standard library
// implementation. To avoid modifying existing code (e.g., replace bits::span with std::span), we essentially alias bits
// to std::span.
namespace bits {
using ::std::span;

// View a trivially-copyable object, such as a packed header, as its bytes without copying it.
template <typename T> span<const uint8_t> object_bytes(const T &value) noexcept {
  static_assert(std::is_trivially_copyable_v<T>, "Requires trivially copyable types");
  return {reinterpret_cast<const uint8_t *>(&value), sizeof(T)};
}
} // namespace bits
