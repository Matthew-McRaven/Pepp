/*
 * Copyright (c) 2025 J. Stanley Warford, Matthew McRaven
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

#include "sim/debug/expr_ast_ops.hpp"
#include "sim/debug/expr_value2.hpp"

TEST_CASE("Value classes & operators", "[scope:debug][kind:unit][arch:*]") {
  using namespace pepp::debug;
  SECTION("Same-width integers") {
    using namespace pepp::debug::operators;
    auto rtti = types::RuntimeTypeInfo();
    VPrimitive lhs = VPrimitive::from_int<int8_t>(-5), rhs = VPrimitive::from_int<int8_t>(16);
    auto type = _typeof(rtti, lhs + rhs);
    REQUIRE(std::holds_alternative<types::TPrimitive>(type));
    CHECK(std::get<types::TPrimitive>(type) == pepp::debug::types::TPrimitive{types::Primitives::i8});
    CHECK(value_bits<int8_t>(lhs + rhs) == (int8_t)11);
    CHECK(value_bits<int8_t>(lhs - rhs) == (int8_t)-21);
    CHECK(value_bits<int8_t>(lhs * rhs) == (int8_t)-80);
  }
}
