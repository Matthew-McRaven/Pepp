
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

#include <catch.hpp>
#include "toolchain/symbol/types.hpp"
#include "toolchain/symbol/value.hpp"

TEST_CASE("Symbol values", "[scope:asm.sym][kind:unit][arch:*]") {
  SECTION("Bit masking") {
    symbol::value::MaskedBits start{.byteCount = 1, .bitPattern = 0xf, .mask = 0x7};
    symbol::value::MaskedBits end{.byteCount = 1, .bitPattern = 0x7, .mask = 0xf};
    CHECK(start == start);
    CHECK(start != end);
    CHECK(start() == end());
  }
  SECTION("Empty") {
    auto value = symbol::value::Empty(0);

    CHECK_NOTHROW(value.value()());
    CHECK(value.value()() == 0);
  }
  SECTION("Deleted") {
    auto value = symbol::value::Deleted();
    CHECK_NOTHROW(value.value()());
    CHECK(value.value()() == 0);
    CHECK(value.type() == symbol::Type::kDeleted);
  }
  // Check that the values on a numeric value can be mutated.
  SECTION("Numeric") {
    symbol::value::MaskedBits start{
        .byteCount = 1,
        .bitPattern = 30,
        .mask = 0xff,
    };
    symbol::value::MaskedBits end{
        .byteCount = 1,
        .bitPattern = 20,
        .mask = 0xff,
    };
    auto value = symbol::value::Constant(start);
    CHECK_NOTHROW(value.value()());
    CHECK(value.value()() == start());
    CHECK_NOTHROW(value.setValue(end));
    CHECK(value.value()() == end());
  }
  // Check that the values on a location value can be mutated.
  SECTION("Location") {
    auto base = 7;
    auto start_offset = 11, end_offset = 13;
    auto value = symbol::value::Location(2, 2, base, start_offset, symbol::Type::kCode);
    CHECK(value.value()() == base + start_offset);
    CHECK_NOTHROW(value.setOffset(end_offset));
    CHECK(value.value()() == base + end_offset);
    REQUIRE(value.relocatable());
  }
  // Can't test internal or external symbol pointer value here, as it will
  // require a symbol table.
}
