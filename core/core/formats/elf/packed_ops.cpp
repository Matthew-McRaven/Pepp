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

#include "core/formats/elf/packed_ops.hpp"
#include "core/math/bitmanip/copy.hpp"

u64 pepp::bts::size_for_layout(const std::vector<pepp::bts::LayoutItem> &layout) noexcept {
  u64 ret = 0;
  for (const auto &item : layout) ret = std::max(ret, item.offset + item.data.size());
  return ret;
}

void pepp::bts::write(std::span<u8> out, const std::vector<LayoutItem> &layout) {
  for (const auto &item : layout) {
    if (item.offset + item.data.size() > out.size())
      throw std::runtime_error("Elf::write: layout item exceeds output size");
    std::span<u8> chunk = out.subspan(item.offset, item.data.size());
    bits::memcpy<u8, u8>(chunk, {item.data});
  }
}
