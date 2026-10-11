/*
 * Copyright (c) 2026. Stanley Warford, Matthew McRaven
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "core/math/bitmanip/strings.hpp"
#include <catch.hpp>
#include <string>
#include <string_view>

TEST_CASE("Escaped character bit ops", "[scope:core][scope:core.math][kind:unit][arch:*]") {
  SECTION("Escape sequences") {
    CHECK(bits::escapedToByte(R"(\n)") == u8('\n'));
    CHECK(bits::escapedToByte(R"(\t)") == u8('\t'));
    CHECK(bits::escapedToByte(R"(\0)") == u8(0));
    CHECK(bits::escapedToByte(R"(\\)") == u8('\\'));
    CHECK(bits::escapedToByte(R"(\')") == u8('\''));
    CHECK(bits::escapedToByte(R"(\")") == u8('"'));
    CHECK(bits::escapedToByte(R"(\x41)") == u8(0x41));
    CHECK(bits::escapedToByte(R"(\xfF)") == u8(0xFF));
  }
  SECTION("Reject invalid escape sequences") {
    CHECK(!bits::escapedToByte(""));
    CHECK(!bits::escapedToByte("ab"));     // More than one character.
    CHECK(!bits::escapedToByte(R"(\q)"));  // Unknown escape.
    CHECK(!bits::escapedToByte(R"(\)"));   // Truncated escape.
    CHECK(!bits::escapedToByte(R"(\x4)")); // Too few hex digits.
    CHECK(!bits::escapedToByte(R"(\x+1)")); // Not hex digits.
  }
}
