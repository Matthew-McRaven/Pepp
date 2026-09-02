/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
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

#include <catch/catch.hpp>
#include "cnl/scaled_integer.h"

TEST_CASE("Fixed-point integers", "[scope:core][scope:core.math][kind:unit][arch:*]") {
  SECTION("Examples from documentation") {
    // 15.9375 * 15.9375 = 254.00390625 ... overflow!
    auto x = cnl::scaled_integer<uint8_t, cnl::power<-4>>{15.9375};
    auto xx1 = cnl::scaled_integer<uint8_t, cnl::power<-4>>{x * x};
    CHECK(xx1 == 14);
    // fixed-point multiplication operator obeys usual promotion and implicit conversion rules
    auto xx = x * x;

    // x*x is promoted to scaled_integer<int, -8>
    static_assert(std::is_same<decltype(xx), cnl::scaled_integer<int, cnl::power<-8>>>::value);
    CHECK(xx == 254.00390625);
  }
}
