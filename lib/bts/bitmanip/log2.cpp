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

#include "./log2.hpp"
#include <bit>
quint8 bits::ceil_log2(quint64 value) {
  if (value == 0) {
    static const char *const e = "Must be non-0";
    qCritical(e);
    throw std::logic_error(e);
  }
  quint64 ceil = std::bit_ceil(value);
  return sizeof(value) * 8 - std::countl_zero(ceil) - 1;
}
