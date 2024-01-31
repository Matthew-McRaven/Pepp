/// The MIT License (MIT)
/// Copyright (c) 2016 Peter Goldsborough
///
/// Permission is hereby granted, free of charge, to any person obtaining a copy
/// of this software and associated documentation files (the "Software"), to
/// deal in the Software without restriction, including without limitation the
/// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
/// sell copies of the Software, and to permit persons to whom the Software is
/// furnished to do so, subject to the following conditions:
///
/// The above copyright notice and this permission notice shall be included in
/// all copies or substantial portions of the Software.
///
/// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
/// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
/// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
/// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
/// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
/// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
/// IN THE SOFTWARE.

#include <catch.hpp>
#include <string>
#include <utility>
#include <vector>

#include "lru/lru.hpp"
#include "tests/move-aware-dummies.hpp"

struct MoveAwarenessTest {
  MoveAwarenessTest() {
    MoveAwareKey::reset();
    MoveAwareValue::reset();
  }

  LRU::Cache<MoveAwareKey, MoveAwareValue> cache;
};

TEST_CASE("MoveAwarenessTest") {
  SECTION("DoesNotMoveForInsert") {
    MoveAwarenessTest t;
    t.cache.insert("x", "y");

    // One construction (right there)
    REQUIRE(MoveAwareKey::forwarding_count == 1);
    REQUIRE(MoveAwareValue::forwarding_count == 1);

    REQUIRE(MoveAwareKey::copy_count == 1);

    // Values only go into the map
    REQUIRE(MoveAwareValue::copy_count == 1);

    // Do this at the end to avoid incrementing the counts
    REQUIRE(t.cache["x"] == "y");
  }
  SECTION("ForwardsValuesWell") {
    MoveAwarenessTest t;
    t.cache.emplace("x", "y");

    // One construction to make the key first
    CHECK(MoveAwareKey::forwarding_count == 1);
    CHECK(MoveAwareValue::forwarding_count == 1);

    CHECK(MoveAwareKey::copy_count == 0);
    CHECK(MoveAwareValue::copy_count == 0);

    REQUIRE(t.cache["x"] == "y");
  }
  SECTION("MovesSingleRValueds") {
    MoveAwarenessTest t;
    t.cache.emplace(std::string("x"), std::string("y"));

    // Move constructions from the string
    CHECK(MoveAwareKey::move_count == 1);
    CHECK(MoveAwareValue::move_count == 1);

    CHECK(MoveAwareKey::non_move_count == 0);
    CHECK(MoveAwareValue::non_move_count == 0);

    CHECK(MoveAwareKey::copy_count == 0);
    CHECK(MoveAwareValue::copy_count == 0);

    REQUIRE(t.cache["x"] == "y");
  }
  SECTION("CopiesSingleLValues") {
    MoveAwarenessTest t;
    std::string x("x");
    std::string y("y");
    t.cache.emplace(x, y);

    // Move constructions from the string
    CHECK(MoveAwareKey::non_move_count == 1);
    CHECK(MoveAwareValue::non_move_count == 1);

    CHECK(MoveAwareKey::move_count == 0);
    CHECK(MoveAwareValue::move_count == 0);

    CHECK(MoveAwareKey::copy_count == 0);
    CHECK(MoveAwareValue::copy_count == 0);

    REQUIRE(t.cache["x"] == "y");
  }
  SECTION("MovesRValueTuples") {
    MoveAwarenessTest t;
    t.cache.emplace(std::piecewise_construct,
                    std::forward_as_tuple(1, 3.14),
                    std::forward_as_tuple(2, 2.718));

    // construct_from_tuple performs one move construction
    // (i.e. construction from rvalues)
    CHECK(MoveAwareKey::move_count == 1);
    CHECK(MoveAwareValue::move_count == 1);

    CHECK(MoveAwareKey::non_move_count == 0);
    CHECK(MoveAwareValue::non_move_count == 0);

    CHECK(MoveAwareKey::copy_count == 0);
    CHECK(MoveAwareValue::copy_count == 0);
  }
  SECTION("MovesLValueTuples") {
    MoveAwarenessTest t;
    int x = 1, z = 2;
    double y = 3.14, w = 2.718;

    t.cache.emplace(std::piecewise_construct,
                    std::forward_as_tuple(x, y),
                    std::forward_as_tuple(z, w));

    // construct_from_tuple will perfom one copy construction
    // (i.e. construction from lvalues)
    CHECK(MoveAwareKey::non_move_count == 1);
    CHECK(MoveAwareValue::non_move_count == 1);

    CHECK(MoveAwareKey::move_count == 0);
    CHECK(MoveAwareValue::move_count == 0);

    CHECK(MoveAwareKey::copy_count == 0);
    CHECK(MoveAwareValue::copy_count == 0);
  }
  SECTION("MovesElementsOutOfRValueRanges") {
    MoveAwarenessTest t;
    std::vector<std::pair<std::string, std::string>> range = {{"x", "y"}};
    t.cache.insert(std::move(range));

    // Move constructions from the string
    CHECK(MoveAwareKey::move_count == 1);
    CHECK(MoveAwareValue::move_count == 1);

    CHECK(MoveAwareKey::non_move_count == 0);
    CHECK(MoveAwareValue::non_move_count == 0);

    CHECK(MoveAwareKey::copy_count == 0);
    CHECK(MoveAwareValue::copy_count == 0);

    REQUIRE(t.cache["x"] == "y");
  }
}
