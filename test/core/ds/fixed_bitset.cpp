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
#include <bitset>
#include <catch.hpp>
#include "core/ds/u64_bitset.hpp"

// Boost already has a test suite
TEST_CASE("FixedBitset", "[scope:core][kind:unit][arch:*]") {
  using namespace pepp;
  using Bitset = FixedBitset<64>;
  u64 testcase_0 = 0x0123456789abcdef;
  std::bitset<64> bit_vec(testcase_0);
  SECTION("All zeroes/ones") {
    Bitset ones = Bitset::ones(), zeros = Bitset::zeros();
    CHECK(ones.all());
    CHECK(!ones.none());
    CHECK(ones.any());

    CHECK(!zeros.all());
    CHECK(zeros.none());
    CHECK(!zeros.any());

    CHECK(ones.count() == 64);
    CHECK(zeros.count() == 0);
    CHECK(ones.size() == zeros.size());

    CHECK(ones == (ones | zeros));
    CHECK(ones == (ones ^ zeros));
    CHECK(zeros == (ones & zeros));
  }
  // Boost already has some useful tests for a (dynamic) bitset, which I replicate in part:.
  // https://github.com/boostorg/dynamic_bitset/blob/develop/test/bitset_test.hpp
  // -----------------------------------------------------------
  //              Copyright (c) 2001 Jeremy Siek
  //      Copyright (c) 2003-2006, 2008, 2025 Gennaro Prota
  //             Copyright (c) 2014 Ahmed Charles
  //            Copyright (c) 2014 Riccardo Marcangelo
  //             Copyright (c) 2018 Evgeny Shulgin
  //
  // Distributed under the Boost Software License, Version 1.0.
  //    (See accompanying file LICENSE_1_0.txt or copy at
  //          http://www.boost.org/LICENSE_1_0.txt)
  //
  // -----------------------------------------------------------
  SECTION("Operator[]") {
    Bitset b(testcase_0);
    std::size_t i, j, k;

    // x = b[i]
    // x = ~b[i]
    for (i = 0; i < b.size(); ++i) {
      bool x = b[i];
      CHECK(x == bit_vec[i]);
      x = ~b[i];
      CHECK(x == !bit_vec[i]);
    }
    Bitset prev(b);

    // b[i] = x
    for (j = 0; j < b.size(); ++j) {
      bool x = !prev[j];
      b[j] = x;
      for (k = 0; k < b.size(); ++k)
        if (j == k) {
          CHECK(b[k] == x);
        } else {
          CHECK(b[k] == prev[k]);
        }
      b[j] = prev[j];
    }
    b.flip();
  }
}