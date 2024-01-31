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

#include <array>
#include <catch.hpp>

#include "lru/lru.hpp"

using namespace LRU;

TEST_CASE("Callback tests") {
  SECTION("HitCallbacksGetCalled") {
    Cache<int, int> cache;
    std::array<int, 3> counts = {0, 0, 0};

    cache.hit_callback([&counts](auto& key, auto& value) { counts[key] += 1; });

    cache.emplace(0, 0);
    cache.emplace(1, 1);
    cache.emplace(2, 2);

    REQUIRE(cache.contains(0));
    CHECK(counts[0] == 1);
    CHECK(counts[1] == 0);
    CHECK(counts[2] == 0);

    cache.find(2);
    CHECK(counts[0] == 1);
    CHECK(counts[1] == 0);
    CHECK(counts[2] == 1);

    cache.lookup(1);
    CHECK(counts[0] == 1);
    CHECK(counts[1] == 1);
    CHECK(counts[2] == 1);

    cache.lookup(0);
    CHECK(counts[0] == 2);
    CHECK(counts[1] == 1);
    CHECK(counts[2] == 1);

    cache.contains(5);
    CHECK(counts[0] == 2);
    CHECK(counts[1] == 1);
    CHECK(counts[2] == 1);
  }
  SECTION("MissCallbacksGetCalled") {
    Cache<int, int> cache;
    std::array<int, 3> counts = {0, 0, 0};

    cache.miss_callback([&counts](auto& key) { counts[key] += 1; });

    cache.emplace(0, 0);

    REQUIRE(cache.contains(0));
    CHECK(counts[0] == 0);
    CHECK(counts[1] == 0);
    CHECK(counts[2] == 0);

    cache.find(2);
    CHECK(counts[0] == 0);
    CHECK(counts[1] == 0);
    CHECK(counts[2] == 1);

    cache.find(1);
    CHECK(counts[0] == 0);
    CHECK(counts[1] == 1);
    CHECK(counts[2] == 1);

    cache.contains(1);
    CHECK(counts[0] == 0);
    CHECK(counts[1] == 2);
    CHECK(counts[2] == 1);
  }
  SECTION("AccessCallbacksGetCalled") {
    Cache<int, int> cache;
    std::array<int, 3> counts = {0, 0, 0};

    cache.access_callback(
        [&counts](auto& key, bool found) { counts[key] += found ? 1 : -1; });

    cache.emplace(0, 0);

    REQUIRE(cache.contains(0));
    CHECK(counts[0] == 1);
    CHECK(counts[1] == 0);
    CHECK(counts[2] == 0);

    cache.find(2);
    CHECK(counts[0] == 1);
    CHECK(counts[1] == 0);
    CHECK(counts[2] == -1);

    cache.find(1);
    CHECK(counts[0] == 1);
    CHECK(counts[1] == -1);
    CHECK(counts[2] == -1);

    cache.contains(1);
    CHECK(counts[0] == 1);
    CHECK(counts[1] == -2);
    CHECK(counts[2] == -1);

    cache.find(0);
    CHECK(counts[0] == 2);
    CHECK(counts[1] == -2);
    CHECK(counts[2] == -1);
  }
  SECTION("CallbacksAreNotCalledAfterBeingCleared") {
    Cache<int, int> cache;
    int hit = 0, miss = 0, access = 0;
    cache.hit_callback([&hit](auto&, auto&) { hit += 1; });
    cache.miss_callback([&miss](auto&) { miss += 1; });
    cache.access_callback([&access](auto&, bool) { access += 1; });

    cache.emplace(0, 0);

    cache.contains(0);
    cache.find(1);

    REQUIRE(hit == 1);
    REQUIRE(miss == 1);
    REQUIRE(access == 2);

    cache.clear_all_callbacks();

    cache.contains(0);
    cache.find(1);
    cache.find(2);

    REQUIRE(hit == 1);
    REQUIRE(miss == 1);
    REQUIRE(access == 2);
  }
}
