
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
#include "core/compile/abstract_value/numeric.hpp"

TEST_CASE("pepp abstract value types", "[scope:core][scope:core.compile][kind:unit][arch:*]") {
  SECTION("Empty") {
    auto value = pepp::ast::SignedDecimal(0, 0);
    CHECK(value.serialized_size() == 0);
    CHECK(value.value_as<u8>() == 0);
  }
  SECTION("Empty with size") {
    auto value = pepp::ast::SignedDecimal(0, 4);
    CHECK(value.serialized_size() == 4);
    CHECK(value.value_as<u32>() == 0);
  }
}
