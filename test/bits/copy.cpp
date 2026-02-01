/*
 * Copyright (c) 2023-2024 J. Stanley Warford, Matthew McRaven
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

#include "core/math/bitmanip/copy.hpp"
#include <QList>
#include <QtEndian>
#include <catch.hpp>
#include <qtypes.h>

using namespace bits;
using vu8 = QList<uint8_t>;

namespace {
void verify(uint8_t *arr, quint16 index, uint8_t golden) { CHECK(arr[index] == golden); }

using T = std::tuple<std::string, quint16, vu8, Order, quint16, Order, vu8>;

// Same length, same endian
// clang-format off
const T _0 = {"same length, big-big, 1-1 byte", 1, vu8{1}, 
	Order::BigEndian, 1, Order::BigEndian, vu8{1}};
const T _1 = {"same length, big-big, 2-2 byte", 2, vu8{0xBE, 0xEF},
    Order::BigEndian, 2, Order::BigEndian, vu8{0xBE, 0xEF}};
const T _2 = {"same length, little-little, 1-1 byte", 1, vu8{1},
    Order::LittleEndian, 1, Order::LittleEndian, vu8{01}};
const T _3 = {"same length, little-little, 2-2 byte", 2, vu8{0xEF, 0xBE},
    Order::LittleEndian, 2, Order::LittleEndian, vu8{0xEF, 0xBE}};

// Source longer, same endian
const T _4 = {"source longer, big-big, 3-2 byte", 3, vu8{0xAA, 0xBB, 0xCC}, 
  Order::BigEndian, 2, Order::BigEndian, vu8{0xBB, 0xCC}};
const T _5 = {"source longer, little-little, 3-2 byte", 3, vu8{0xCC, 0xBB, 0xAA}, 
  Order::LittleEndian, 2, Order::LittleEndian, vu8{0xCC, 0xBB}};
  
  // Dest longer, same endian
const T _6 = {"dest longer, big-big, 2-3 byte", 2, vu8{0xAA, 0xBB}, 
  Order::BigEndian, 3, Order::BigEndian, vu8{0x00, 0xAA, 0xBB}};
const T _7 = {"dest longer, little-little, 2-3 byte", 2, vu8{0xBB, 0xAA}, 
  Order::LittleEndian, 3, Order::LittleEndian, vu8{0xBB, 0xAA, 0x00}};
const T _8 = {"dest longer, big-big, 2-3 byte", 2, vu8{0xAA, 0xBB}, 
  Order::BigEndian, 3, Order::BigEndian, vu8{0x00, 0xAA, 0xBB}};
const T _9 = {"dest longer, little-little, 2-3 byte", 2, vu8{0xBB, 0xAA}, 
  Order::LittleEndian, 3, Order::LittleEndian, vu8{0xBB, 0xAA, 0x00}};
  
// Same length, mixed endian
const T _10 = {"same length, little-big, 1-1 byte", 1, vu8{1}, 
  Order::LittleEndian, 1, Order::BigEndian, vu8{01}};
const T _11 = {"same length, little-big, 2-2 byte", 2, vu8{0xEF, 0xBE}, 
  Order::LittleEndian, 2, Order::BigEndian, vu8{0xBE, 0xEF}};
const T _12 = {"same length, big-little, 1-1 byte", 1, vu8{1}, 
  Order::BigEndian, 1, Order::LittleEndian, vu8{01}};
const T _13 = {"same length, big-little, 2-2 byte", 2, vu8{0xBE, 0xEF}, 
  Order::BigEndian, 2, Order::LittleEndian, vu8{0xEF, 0xBE}};
  
// Source longer, different endian
const T _14 = {"source longer, little-big, 3-2 byte", 3, vu8{0xCC, 0xBB, 0xAA}, 
  Order::LittleEndian, 2, Order::BigEndian, vu8{0xBB, 0xCC}};
const T _15 = {"source longer, big-little, 3-2 byte", 3, vu8{0xAA, 0xBB, 0xCC}, 
  Order::BigEndian, 2, Order::LittleEndian, vu8{0xCC, 0xBB}};
  
// Dest longer, different endian
const T _16 = {"dest longer, little-big, 2-3 byte", 2, vu8{0xBB, 0xAA}, 
  Order::LittleEndian, 3, Order::BigEndian, vu8{0x00, 0xAA, 0xBB}};
const T _17 = {"dest longer, big-little, 2-3 byte", 2, vu8{0xAA, 0xBB}, 
  Order::BigEndian, 3, Order::LittleEndian, vu8{0xBB, 0xAA, 0x00}};
}

// clang-format on

TEST_CASE("Copy bits", "[scope:bits][kind:unit][arch:*]") {
  auto [caseName, srcLen, srcData, srcOrder, destLen, destOrder, destGolden] =
      GENERATE(table<std::string, quint16, vu8, Order, quint16, Order, vu8>(
          {_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17}));
  DYNAMIC_SECTION(caseName) {
    auto dest_le = quint64_le{0};
    auto dest_be = quint64_be{0};
    uint8_t *dest = nullptr;
    if (destOrder == Order::BigEndian) dest = reinterpret_cast<uint8_t *>(&dest_be);
    else if (destOrder == Order::LittleEndian) dest = reinterpret_cast<uint8_t *>(&dest_le);
    auto src = srcData.constData();
    memcpy_endian({dest, destLen}, destOrder, {src, srcLen}, srcOrder);
    for (int it = 0; it < destLen; it++) verify(dest, it, destGolden[it]);
  }
}
