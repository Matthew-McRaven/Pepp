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
#include <algorithm>
#include <array>
#include "core/sim/api/memory.hpp"
#include "core/sim/memory/errors.hpp"

// A default implementation for devices without permission support, which untraceably writes + zero-fill.
void Target::load(AddressSpan span, bits::span<const u8> data, Access) {
  static const Operation load_op{Operation::Type::Application, Operation::Kind::data};
  if (!span.valid()) return;
  const u64 size = pepp::core::size_inclusive(span);
  // The data would run past the end of the span it was given, so report the first address which does not fit.
  if (data.size() > size) throw Error(Error::Type::OOBAccess, static_cast<Address>(span.lower() + size));
  else if (!data.empty()) write(span.lower(), data, load_op);
  // Fill remaining bytes of the span with zeroes.
  static constexpr std::size_t chunk_size = 256;
  static const std::array<u8, chunk_size> zeroes{};
  for (u64 offset = data.size(); offset < size;) {
    const auto count = std::min<u64>(chunk_size, size - offset);
    write(static_cast<Address>(span.lower() + offset), bits::span<const u8>(zeroes.data(), count), load_op);
    offset += count;
  }
}
