
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

#include "core/math/bitmanip/mask.hpp"
#include <catch/catch.hpp>

TEST_CASE("Masked bits", "[scope:core][scope:core.math][kind:unit][arch:*]") {
  SECTION("Bit masking") {
    bits::MaskedBits start{.byteCount = 1, .bitPattern = 0xf, .mask = 0x7};
    bits::MaskedBits end{.byteCount = 1, .bitPattern = 0x7, .mask = 0xf};
    CHECK(start == start);
    CHECK(start != end);
    CHECK(start() == end());
  }
}
