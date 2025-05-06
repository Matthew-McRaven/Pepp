/*
 * Copyright (c) 2023 J. Stanley Warford, Matthew McRaven
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

#include "./helpers.hpp"
#include "enums/isa/pep10.hpp"
#include "enums/isa/pep9.hpp"

template <> quint8 targets::isa::packCSR<isa::Pep10>(bool n, bool z, bool v, bool c) {
  return (n << 3) | (z << 2) | (v << 1) | (c << 0);
}
template <> quint8 targets::isa::packCSR<isa::Pep9>(bool n, bool z, bool v, bool c) {
  return (n << 3) | (z << 2) | (v << 1) | (c << 0);
}

template <> std::tuple<bool, bool, bool, bool> targets::isa::unpackCSR<isa::Pep10>(quint8 value) {
  return {value & 0b1000, value & 0b0100, value & 0b0010, value & 0b0001};
}

template <> std::tuple<bool, bool, bool, bool> targets::isa::unpackCSR<isa::Pep9>(quint8 value) {
  return {value & 0b1000, value & 0b0100, value & 0b0010, value & 0b0001};
}
