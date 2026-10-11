
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
  SECTION("Masks and sign extension") {
    STATIC_REQUIRE(bits::mask(1) == 0xFF);
    STATIC_REQUIRE(bits::mask(8) == ~0ull);
    STATIC_REQUIRE(bits::sign_extend(0x7F, 1) == 0x7F);
    STATIC_REQUIRE(bits::sign_extend(0x80, 1) == 0xFFFF'FFFF'FFFF'FF80ull);
    STATIC_REQUIRE(bits::sign_extend(0x1234'FFFE, 2) == 0xFFFF'FFFF'FFFF'FFFEull); // Bits above the width are ignored.
    STATIC_REQUIRE(bits::sign_extend(0x8000'0000'0000'0000ull, 8) == 0x8000'0000'0000'0000ull);
  }
}
