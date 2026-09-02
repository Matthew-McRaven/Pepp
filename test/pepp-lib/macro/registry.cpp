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

#include "toolchain/macro/registry.hpp"
#include <catch.hpp>
#include "toolchain/macro/declaration.hpp"
TEST_CASE("Macro registry", "[scope:macro][kind:unit][arch:*]") {
  SECTION("Register Macros") {
    macro::Registry reg;
    auto parsed = QSharedPointer<macro::Declaration>::create("alpha", 0, "body", "none");
    auto registered = reg.registerMacro(macro::types::Type::Core, parsed);
    CHECK(registered != nullptr);
    CHECK(registered->contents() == parsed);
  }
  SECTION("Find by name") {
    macro::Registry reg;
    auto parsed = QSharedPointer<macro::Declaration>::create("alpha", 0, "body", "none");
    auto registered = reg.registerMacro(macro::types::Type::Core, parsed);
    CHECK(registered != nullptr);
    CHECK(registered->contents() == parsed);
    CHECK(reg.findMacro("alpha") == registered);
  }
  SECTION("Reject duplicate names") {
    macro::Registry reg;
    auto parsed = QSharedPointer<macro::Declaration>::create("alpha", 0, "body", "none");
    auto parsed2 = QSharedPointer<macro::Declaration>::create("alpha", 0, "body", "none");
    auto registered = reg.registerMacro(macro::types::Type::Core, parsed);
    CHECK(registered != nullptr);
    CHECK(reg.registerMacro(macro::types::Type::Core, parsed2) == nullptr);
  }
  SECTION("Delineates macro types") {
    macro::Registry reg;
    auto parsed = QSharedPointer<macro::Declaration>::create("alpha", 0, "body", "none");
    auto parsed2 = QSharedPointer<macro::Declaration>::create("beta", 0, "body", "none");
    REQUIRE(reg.registerMacro(macro::types::Type::Core, parsed) != nullptr);
    REQUIRE(reg.registerMacro(macro::types::Type::System, parsed2) != nullptr);
    REQUIRE(reg.findMacrosByType(macro::types::Type::Core).size() == 1);
    REQUIRE(reg.findMacrosByType(macro::types::Type::System).size() == 1);
    REQUIRE(reg.findMacrosByType(macro::types::Type::User).empty());
  }
}
