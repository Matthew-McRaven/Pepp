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

#include "core/formats/dwarf/cursor.hpp"
#include <algorithm>
#include "core/math/bitmanip/leb128.hpp"

void pepp::bts::DwarfCursor::fail(const char *why) noexcept {
  if (error == nullptr) error = why;
  offset = data.size();
}

bits::span<const u8> pepp::bts::DwarfCursor::bytes(size_t length) noexcept {
  if (!ok()) return {};
  if (remaining() < length) {
    fail("unexpected end of data");
    return {};
  }
  auto ret = data.subspan(offset, length);
  offset += length;
  return ret;
}

u64 pepp::bts::DwarfCursor::uleb() noexcept {
  if (!ok()) return 0;
  unsigned length = 0;
  const char *why = nullptr;
  const auto ret = bits::decodeULEB128(data.data() + offset, &length, data.data() + data.size(), &why);
  if (why != nullptr) {
    fail(why);
    return 0;
  }
  offset += length;
  return ret;
}

i64 pepp::bts::DwarfCursor::sleb() noexcept {
  if (!ok()) return 0;
  unsigned length = 0;
  const char *why = nullptr;
  const auto ret = bits::decodeSLEB128(data.data() + offset, &length, data.data() + data.size(), &why);
  if (why != nullptr) {
    fail(why);
    return 0;
  }
  offset += length;
  return ret;
}

std::string_view pepp::bts::DwarfCursor::cstr() noexcept {
  if (!ok()) return {};
  const auto rest = data.subspan(offset);
  const auto nul = std::find(rest.begin(), rest.end(), u8{0});
  if (nul == rest.end()) {
    fail("unterminated string");
    return {};
  }
  const auto length = static_cast<size_t>(nul - rest.begin());
  offset += length + 1;
  return {reinterpret_cast<const char *>(rest.data()), length};
}
