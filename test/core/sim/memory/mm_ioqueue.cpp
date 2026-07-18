/*
 * Copyright (c) 2024 J. Stanley Warford, Matthew McRaven
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
#include "core/sim/memory/io/mm.hpp"

TEST_CASE("MemoryMappedReg IOQueue", "[scope:core][scope:core.sim][kind:int][arch:*]") {

  SECTION("Ops on empty queue") {
    auto q = IOQueue{};
    CHECK(q.size() == 0);
    CHECK(q.empty());
    CHECK(q.end().at_end());
    CHECK(q.begin().at_end());
    CHECK(q.begin() == q.end());
  }

  SECTION("Insert one item") {
    auto q = IOQueue{};
    CHECK(q.size() == 0);
    CHECK(q.empty());
    CHECK(q.end().at_end());
    CHECK(q.begin() == q.end());
    q.push(17);
    CHECK(q.size() == 1);
    CHECK(!q.empty());
    CHECK(q.end().at_end());
    CHECK(q.begin() != q.end());
    CHECK(*q.begin() == 17);
    CHECK(q.begin().value_or(0) == 17);
  }

  SECTION("Insert multiple item") {
    auto q = IOQueue{};
    int it = 0;
    u8 values[] = {17, 18, 20};
    q.push(values[0]);
    q.push(values[1]);
    q.push(values[2]);

    for (const auto v : q) {
      CHECK(v == values[it++]);
    }
  }
}
