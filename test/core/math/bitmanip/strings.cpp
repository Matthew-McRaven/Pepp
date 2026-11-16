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

TEST_CASE("Extract C strings from span", "[scope:core][scope:core.math][kind:unit][arch:*]") {
  using namespace std::string_view_literals;
  const auto view = [](std::string_view sv) { return bits::until_nul(bits::span<const char>(sv.data(), sv.size())); };
  CHECK(view("abc\0def"sv) == "abc");
  CHECK(view("abc"sv) == "abc");
  CHECK(view("\0abc"sv).empty());
  CHECK(view(""sv).empty());
}

TEST_CASE("Integer literals", "[scope:core][scope:core.math][kind:unit][arch:*]") {
  SECTION("C") {
    CHECK(bits::c_integer("42") == bits::ParsedInteger{42, 10});
    CHECK(bits::c_integer("0XfF") == bits::ParsedInteger{0xFF, 16});
    CHECK(bits::c_integer("0xFFFFFFFFFFFFFFFF") == bits::ParsedInteger{~u64{0}, 16});
    CHECK(bits::c_integer("010") == bits::ParsedInteger{10, 10});
    for (const auto *word : {"", "0x", "12ab", "-5", "18446744073709551616"}) {
      INFO(word);
      CHECK(!bits::c_integer(word));
    }
  }
  SECTION("ld") {
    CHECK(bits::ld_integer("$10K") == bits::ParsedInteger{0x4000, 16});
    CHECK(bits::ld_integer("0x10M") == bits::ParsedInteger{0x1000000, 16});
    CHECK(bits::ld_integer("4k") == bits::ParsedInteger{4096, 10});
    CHECK(bits::ld_integer("010") == bits::ParsedInteger{8, 8});
    CHECK(bits::ld_integer("10h") == bits::ParsedInteger{0x10, 16});
    CHECK(bits::ld_integer("ax") == bits::ParsedInteger{0xA, 16});
    CHECK(bits::ld_integer("deadh") == bits::ParsedInteger{0xDEAD, 16});
    CHECK(bits::ld_integer("101b") == bits::ParsedInteger{5, 2});
    CHECK(bits::ld_integer("17o") == bits::ParsedInteger{15, 8});
    CHECK(bits::ld_integer("25d") == bits::ParsedInteger{25, 10});
    CHECK(bits::ld_integer("1bh") == bits::ParsedInteger{0x1B, 16});
    CHECK(bits::ld_integer("0x") == bits::ParsedInteger{0, 16}); // 0 with an x suffix.
    // Names, invalid digits for the base, and values past 64 bits.
    for (const auto *word : {"bad", "fade", "08", "0b11", "12b", "1fd", "19o", "$", "17592186044416M"}) {
      INFO(word);
      CHECK(!bits::ld_integer(word));
    }
  }
}
