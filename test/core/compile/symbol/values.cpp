
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
#include "core/compile/symbol/value.hpp"
#include "core/math/bitmanip/mask.hpp"

TEST_CASE("pepp::core symbol values", "[scope:core][scope:core.compile][kind:unit][arch:*]") {
  SECTION("Empty") {
    auto value = pepp::core::symbol::EmptyValue(0);

    CHECK_NOTHROW(value.value()());
    CHECK(value.value()() == 0);
  }
  SECTION("Deleted") {
    auto value = pepp::core::symbol::DeletedValue();
    CHECK_NOTHROW(value.value()());
    CHECK(value.value()() == 0);
    CHECK(value.type() == pepp::core::symbol::Type::Deleted);
  }
  // Check that the values on a numeric value can be mutated.
  SECTION("Constants") {
    bits::MaskedBits start{
        .byteCount = 1,
        .bitPattern = 30,
        .mask = 0xff,
    };
    bits::MaskedBits end{
        .byteCount = 1,
        .bitPattern = 20,
        .mask = 0xff,
    };
    auto value = pepp::core::symbol::ConstantValue(start);
    CHECK_NOTHROW(value.value()());
    CHECK(value.value()() == start());
    CHECK_NOTHROW(value.set_value(end));
    CHECK(value.value()() == end());
  }
  // Check that the values on a location value can be mutated.
  SECTION("Location") {
    auto base = 7;
    auto start_offset = 11, end_offset = 13;
    auto value = pepp::core::symbol::LocationValue(2, 2, base, start_offset, pepp::core::symbol::Type::Code);
    CHECK(value.value()() == base + start_offset);
    CHECK_NOTHROW(value.set_offset(end_offset));
    CHECK(value.value()() == base + end_offset);
    CHECK(value.effective_address() == value.value().bitPattern);
  }
}
