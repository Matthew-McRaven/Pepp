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

#include "sim/debug/expr_parser.hpp"
#include "sim/debug/expr_rtti.hpp"

TEST_CASE("Watch expression types", "[scope:debug][kind:unit][arch:*]") {
  using namespace pepp::debug;
  using P = types::Primitives;
  SECTION("Redefinitions have same handle") {
    types::RuntimeTypeInfo rtti;
    types::NamedTypeInfo nti(rtti);
    auto _1 = nti.register_name("mystruct");
    auto _2 = nti.register_name("mystruct");
    CHECK(_1.first);
    CHECK(!_2.first);
    CHECK(_1.second == _2.second);
  }
  SECTION("Lookups to same handle work") {
    types::RuntimeTypeInfo rtti;
    types::NamedTypeInfo nti(rtti);
    auto _1 = nti.register_name("mystruct");
    CHECK(_1.first);
    auto maybe_hnd = nti.handle("mystruct");
    REQUIRE(maybe_hnd);
    CHECK(*maybe_hnd == _1.second);
  }
  SECTION("Type definitions stick to handles") {
    types::RuntimeTypeInfo rtti;
    types::NamedTypeInfo nti(rtti);
    auto _1 = nti.register_name("mystruct");
    CHECK(_1.first);
    auto hnd = _1.second;
    CHECK(nti.type(hnd).first == 0);
    CHECK(nti.type(hnd).second == types::Never{});
  }
}
