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
#include <chrono>
#include <string>
#include <thread>

#include "lru/lru.hpp"

using namespace LRU;
using namespace std::chrono_literals;
using CacheType = Cache<std::string, int>;
CacheType cache;

TEST_CASE("IteratorTest") {
  SECTION("UnorderedIteratorsAreCompatibleAsExpected") {
    CacheType cache;
    cache.emplace("one", 1);

    // Move construction
    CacheType::iterator first(cache.begin());

    // Copy construction
    CacheType::iterator second(first);

    // Copy assignment
    CacheType::iterator third;
    third = second;

    // Move construction from non-const to const
    CacheType::const_iterator first_const(std::move(first));

    // Copy construction from non-const to const
    CacheType::const_iterator second_const(second);

    // Copy assignment
    CacheType::const_iterator third_const;
    third_const = third;
  }

  SECTION("UnorderedIteratorsDoNotChangeTheOrderOfElements") {
    CacheType cache;
    cache.capacity(2);
    cache.insert({{"one", 1}});

    auto begin = cache.begin();

    cache.emplace("two", 2);

    REQUIRE(begin->first == "one");
    cache.emplace("three", 3);

    CHECK_FALSE(cache.contains("one"));
    CHECK(cache.contains("two"));
    CHECK(cache.contains("three"));

    REQUIRE(cache.back() == "two");
    REQUIRE(cache.front() == "three");
  }
  SECTION("IsValidReturnsTrueForValidIterators") {
    CacheType cache;
    cache.emplace("one", 1);
    cache.emplace("two", 1);

    // TODO: const conversion is broken
    auto iterator = cache.cbegin();
    CHECK(cache.is_valid(iterator));
    CHECK(cache.is_valid(++iterator));

    auto unordered_iterator = cache.begin();
    CHECK(cache.is_valid(unordered_iterator));
    CHECK(cache.is_valid(++unordered_iterator));
  }
  SECTION("ThrowIfInvalidThrowsAsExpected") {
    CacheType cache;
    CHECK_THROWS_AS(cache.throw_if_invalid(cache.begin()),
                    LRU::Error::InvalidIterator);
    CHECK_THROWS_AS(cache.throw_if_invalid(cache.end()),
                    LRU::Error::InvalidIterator);
  }
  SECTION("ThrowIfInvalidThrowsAsExpected") {
    CacheType cache;

    CHECK_THROWS_AS(cache.throw_if_invalid(cache.begin()),
                    LRU::Error::InvalidIterator);
    CHECK_THROWS_AS(cache.throw_if_invalid(cache.end()),
                    LRU::Error::InvalidIterator);
  }
  SECTION("IsValidReturnsFalseForInvalidIterators") {
    CacheType cache;

    CHECK_FALSE(cache.is_valid(cache.begin()));
    CHECK_FALSE(cache.is_valid(cache.end()));
  }
}
