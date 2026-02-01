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

#include <QList>
#include <catch.hpp>
#include <qtypes.h>
#include "core/math/bitmanip/copy.hpp"
#include "core/math/bitmanip/strings.hpp"
namespace {
static const QList<bits::SeparatorRule> rules = {{.skipFirst = false, .separator = ' ', .modulus = 1}};
}
TEST_CASE("String bit ops", "[scope:bits][kind:unit][arch:*]") {
  quint8 src[] = {0x00, 0xFE, 0xED, 0xBE, 0xEF};
  char dst[sizeof(src) * 3];
  auto dstSpan = std::span{dst};
  auto golden = "00 FE ED BE EF ";
  /*quint8 golden[sizeof(dst)] = {0x30, 0x30, 0x20, 0x46, 0x45,
                                0x20, 0x45, 0x44, 0x20, 0x42,
                                0x45, 0x20, 0x45, 0x46, 0x20};*/
  bits::memclr(dstSpan);
  CHECK(bits::bytesToAsciiHex({dst, sizeof(dst)}, {src, sizeof(src)}, rules) == sizeof(dst));
  QString dstStr = QString::fromLocal8Bit(reinterpret_cast<const char *>(dst), sizeof(dst));
  CHECK(dstStr == golden);
}
