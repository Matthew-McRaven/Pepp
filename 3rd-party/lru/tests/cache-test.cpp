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

#include <algorithm>
#include <catch.hpp>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "lru/lru.hpp"

using namespace LRU;

struct CacheTest {
  using CacheType = Cache<std::string, int>;

  template <typename Cache, typename Range>
  bool is_equal_to_range(const Cache& cache, const Range& range) {
    if (cache.size() != range.size()) return false;
    for (auto item : range) {
      if (auto i = cache.const_find(item.first); i == cache.cend()) {
        return false;
      } else if (i->second.value != item.second)
        return false;
    }
    return true;
  }

  CacheType cache;
};

TEST_CASE("CacheConstructionTest") {
  SECTION("IsConstructibleFromInitializerList") {
    Cache<std::string, int> cache = {
        {"one", 1},
        {"two", 2},
        {"three", 3},
    };
    CHECK(!cache.is_empty());
    CHECK(cache.size() == 3);
    CHECK(cache["one"] == 1);
    CHECK(cache["two"] == 2);
    CHECK(cache["three"] == 3);
  }
  SECTION("IsConstructibleFromInitializerListWithCapacity") {
    // clang-format off
    Cache<std::string, int> cache(2, {
                                      {"one", 1}, {"two", 2}, {"three", 3},
                                      });
    // clang-format on

    CHECK(!cache.is_empty());
    CHECK(cache.size() == 2);
    CHECK(!cache.contains("one"));
    CHECK(cache["two"] == 2);
    CHECK(cache["three"] == 3);
  }
  SECTION("IsConstructibleFromRange") {
    const std::vector<std::pair<std::string, int>> range = {
        {"one", 1}, {"two", 2}, {"three", 3}};
    Cache<std::string, int> cache(range);
    CHECK(!cache.is_empty());
    CHECK(cache.size() == 3);
    CHECK(cache["one"] == 1);
    CHECK(cache["two"] == 2);
    CHECK(cache["three"] == 3);
  }
  SECTION("IsConstructibleFromIterators") {
    std::vector<std::pair<std::string, int>> range = {
        {"one", 1}, {"two", 2}, {"three", 3}};

    Cache<std::string, int> cache(range.begin(), range.end());

    CHECK(!cache.is_empty());
    CHECK(cache.size() == 3);
    CHECK(cache["one"] == 1);
    CHECK(cache["two"] == 2);
    CHECK(cache["three"] == 3);
  }
  SECTION("CapacityIsMaxOfInternalDefaultAndIteratorDistance") {
    std::vector<std::pair<std::string, int>> range = {
        {"one", 1}, {"two", 2}, {"three", 3}};

    Cache<std::string, int> cache(range.begin(), range.end());

    CHECK(cache.capacity() == Internal::DEFAULT_CAPACITY);

    for (int i = 0; i < Internal::DEFAULT_CAPACITY; ++i) {
      range.emplace_back(std::to_string(i), i);
    }

    cache = std::move(range);
    CHECK(cache.capacity() == range.size());

    Cache<std::string, int> cache2(range.begin(), range.end());
    CHECK(cache2.capacity() == range.size());
  }
  SECTION("UsesCustomHashFunction") {
    using MockHash = std::function<int(int)>;

    std::size_t mock_hash_call_count = 0;
    MockHash mock_hash = [&mock_hash_call_count](int value) {
      mock_hash_call_count += 1;
      return value;
    };

    Cache<int, int, decltype(mock_hash)> cache(128, mock_hash);

    CHECK(mock_hash_call_count == 0);

    cache.contains(5);
    CHECK(mock_hash_call_count == 1);
  }
  SECTION("UsesCustomKeyEqual") {
    using MockCompare = std::function<bool(int, int)>;

    std::size_t mock_equal_call_count = 0;
    MockCompare mock_equal = [&mock_equal_call_count](int a, int b) {
      mock_equal_call_count += 1;
      return a == b;
    };

    Cache<int, int, std::hash<int>, decltype(mock_equal)> cache(
        128, std::hash<int>(), mock_equal);

    CHECK(mock_equal_call_count == 0);

    cache.insert(5, 1);
    REQUIRE(cache.contains(5));
    CHECK(mock_equal_call_count == 1);
  }
  SECTION("ContainsAfterInsertion") {
    CacheTest t;
    REQUIRE(t.cache.is_empty());

    for (std::size_t i = 1; i <= 100; ++i) {
      const auto key = std::to_string(i);
      t.cache.insert(key, i);
      CHECK(t.cache.size() == i);
      CHECK(t.cache.contains(key));
    }

    CHECK_FALSE(t.cache.is_empty());
  }

  SECTION("ContainsAfteEmplacement") {
    CacheTest t;
    REQUIRE(t.cache.is_empty());

    for (std::size_t i = 1; i <= 100; ++i) {
      const auto key = std::to_string(i);
      t.cache.emplace(key, i);
      CHECK(t.cache.size() == i);
      CHECK(t.cache.contains(key));
    }

    CHECK_FALSE(t.cache.is_empty());
  }

  SECTION("RemovesLRUElementWhenFull") {
    CacheTest t;
    t.cache.capacity(2);
    REQUIRE(t.cache.capacity() == 2);

    t.cache.emplace("one", 1);
    t.cache.emplace("two", 2);
    REQUIRE(t.cache.size() == 2);
    REQUIRE(t.cache.contains("one"));
    REQUIRE(t.cache.contains("two"));


    t.cache.emplace("three", 3);
    CHECK(t.cache.size() == 2);
    CHECK(t.cache.contains("two"));
    CHECK(t.cache.contains("three"));
    CHECK_FALSE(t.cache.contains("one"));
  }
  SECTION("LookupReturnsTheRightValue") {
    CacheTest t;
    for (std::size_t i = 1; i <= 10; ++i) {
      const auto key = std::to_string(i);
      t.cache.emplace(key, i);
      REQUIRE(t.cache.size() == i);
      CHECK(t.cache.lookup(key) == i);
      CHECK(t.cache[key] == i);
    }
  }
  SECTION("LookupOnlyThrowsWhenKeyNotFound") {
    CacheTest t;
    t.cache.emplace("one", 1);

    REQUIRE(t.cache.size() == 1);
    CHECK(t.cache.lookup("one") == 1);

    CHECK_THROWS_AS(t.cache.lookup("two"), LRU::Error::KeyNotFound);
    CHECK_THROWS_AS(t.cache.lookup("three"), LRU::Error::KeyNotFound);

    t.cache.emplace("two", 2);
    CHECK(t.cache.lookup("two") == 2);
  }
  SECTION("SizeIsUpdatedProperly") {
    CacheTest t;
    REQUIRE(t.cache.size() == 0);

    for (std::size_t i = 1; i <= 10; ++i) {
      t.cache.emplace(std::to_string(i), i);
      // Use REQUIRE and not CHECK to terminate the loop early
      REQUIRE(t.cache.size() == i);
    }

    for (std::size_t i = 10; i >= 1; --i) {
      REQUIRE(t.cache.size() == i);
      t.cache.erase(std::to_string(i));
      // Use REQUIRE and not CHECK to terminate the loop early
    }

    CHECK(t.cache.size() == 0);
  }
  SECTION("SpaceLeftWorks") {
    CacheTest t;
    t.cache.capacity(10);
    REQUIRE(t.cache.size() == 0);

    for (std::size_t i = 10; i >= 1; --i) {
      CHECK(t.cache.space_left() == i);
      t.cache.emplace(std::to_string(i), i);
    }

    CHECK(t.cache.space_left() == 0);
  }
  SECTION("IsEmptyWorks") {
    CacheTest t;
    REQUIRE(t.cache.is_empty());
    t.cache.emplace("one", 1);
    CHECK_FALSE(t.cache.is_empty());
    t.cache.clear();
    CHECK(t.cache.is_empty());
  }
  SECTION("IsFullWorks") {
    CacheTest t;
    REQUIRE_FALSE(t.cache.is_full());
    t.cache.capacity(0);
    REQUIRE(t.cache.is_full());

    t.cache.capacity(2);
    t.cache.emplace("one", 1);
    CHECK_FALSE(t.cache.is_full());
    t.cache.emplace("two", 1);
    CHECK(t.cache.is_full());

    t.cache.clear();
    CHECK_FALSE(t.cache.is_full());
  }
  SECTION("CapacityCanBeAdjusted") {
    CacheTest t;
    t.cache.capacity(10);

    REQUIRE(t.cache.capacity() == 10);

    for (std::size_t i = 0; i < 10; ++i) {
      t.cache.emplace(std::to_string(i), i);
    }

    REQUIRE(t.cache.size() == 10);

    t.cache.emplace("foo", 0xdeadbeef);
    CHECK(t.cache.size() == 10);

    t.cache.capacity(11);
    REQUIRE(t.cache.capacity() == 11);

    t.cache.emplace("bar", 0xdeadbeef);
    CHECK(t.cache.size() == 11);

    t.cache.capacity(5);
    CHECK(t.cache.capacity() == 5);
    CHECK(t.cache.size() == 5);

    t.cache.capacity(0);
    CHECK(t.cache.capacity() == 0);
    CHECK(t.cache.size() == 0);

    t.cache.capacity(128);
    CHECK(t.cache.capacity() == 128);
    CHECK(t.cache.size() == 0);
  }
  SECTION("EraseErasesAndReturnsTrueWhenElementContained") {
    CacheTest t;
    t.cache.emplace("one", 1);
    REQUIRE(t.cache.contains("one"));

    CHECK(t.cache.erase("one"));
    CHECK_FALSE(t.cache.contains("one"));
  }
  SECTION("EraseReturnsFalseWhenElementNotContained") {
    CacheTest t;
    REQUIRE_FALSE(t.cache.contains("one"));
    CHECK_FALSE(t.cache.erase("one"));
  }
  SECTION("ClearRemovesAllElements") {
    CacheTest t;
    REQUIRE(t.cache.is_empty());

    t.cache.emplace("one", 1);
    CHECK_FALSE(t.cache.is_empty());

    t.cache.clear();
    CHECK(t.cache.is_empty());
  }
  SECTION("ShrinkAdjustsSizeWell") {
    CacheTest t;
    t.cache.emplace("one", 1);
    t.cache.emplace("two", 2);

    REQUIRE(t.cache.size() == 2);

    t.cache.shrink(1);

    CHECK(t.cache.size() == 1);

    t.cache.emplace("three", 2);
    t.cache.emplace("four", 3);

    REQUIRE(t.cache.size() == 3);

    t.cache.shrink(1);

    CHECK(t.cache.size() == 1);

    t.cache.shrink(0);

    CHECK(t.cache.is_empty());
  }
  SECTION("ShrinkDoesNothingWhenRequestedSizeIsGreaterThanCurrent") {
    CacheTest t;
    t.cache.emplace("one", 1);
    t.cache.emplace("two", 2);

    REQUIRE(t.cache.size() == 2);

    t.cache.shrink(50);

    CHECK(t.cache.size() == 2);
  }
  SECTION("ShrinkRemovesLRUElements") {
    CacheTest t;
    t.cache.emplace("one", 1);
    t.cache.emplace("two", 2);
    t.cache.emplace("three", 3);

    REQUIRE(t.cache.size() == 3);

    t.cache.shrink(2);

    CHECK(t.cache.size() == 2);
    CHECK_FALSE(t.cache.contains("one"));
    CHECK(t.cache.contains("two"));
    CHECK(t.cache.contains("three"));

    t.cache.shrink(1);

    CHECK(t.cache.size() == 1);
    CHECK_FALSE(t.cache.contains("one"));
    CHECK_FALSE(t.cache.contains("two"));
    CHECK(t.cache.contains("three"));
  }
  SECTION("CanInsertIterators") {
    CacheTest t;
    using Range = std::vector<std::pair<std::string, int>>;
    Range range = {{"one", 1}, {"two", 2}, {"three", 3}};

    CHECK(t.cache.insert(range.begin(), range.end()) == 3);
    CHECK(t.is_equal_to_range(t.cache, range));

    Range range2 = {{"one", 1}, {"four", 4}};

    CHECK(t.cache.insert(range2.begin(), range2.end()) == 1);
    // clang-format off
  	CHECK(t.is_equal_to_range(t.cache, Range({
      {"two", 2}, {"three", 3}, {"one", 1}, {"four", 4}
 		})));
    // clang-format on
  }
  SECTION("CanInsertRange") {
    CacheTest t;
    std::vector<std::pair<std::string, int>> range = {
        {"one", 1}, {"two", 2}, {"three", 3}};

    t.cache.insert(range);
    CHECK(t.is_equal_to_range(t.cache, range));
  }
  SECTION("CanInsertList") {
    CacheTest t;
    std::initializer_list<std::pair<std::string, int>> list = {
        {"one", 1}, {"two", 2}, {"three", 3}};

    // Do it like this, just to verify that template deduction fails if only
    // the range function exists and no explicit overload for the initializer
    // list
    t.cache.insert({{"one", 1}, {"two", 2}, {"three", 3}});
    CHECK(t.is_equal_to_range(t.cache, list));
  }
  SECTION("ResultIsCorrectForInsert") {
    CacheTest t;
    auto result = t.cache.insert("one", 1);

    CHECK(result.was_inserted());
    CHECK(result);

    CHECK(result.iterator() == t.cache.begin());

    result = t.cache.insert("one", 1);

    CHECK_FALSE(result.was_inserted());
    CHECK_FALSE(result);

    CHECK(result.iterator() == t.cache.begin());
  }
  SECTION("ResultIsCorrectForEmplace") {
    CacheTest t;
    auto result = t.cache.emplace("one", 1);

    CHECK(result.was_inserted());
    CHECK(result);

    CHECK(result.iterator() == t.cache.begin());

    result = t.cache.emplace("one", 1);

    CHECK_FALSE(result.was_inserted());
    CHECK_FALSE(result);

    CHECK(result.iterator() == t.cache.begin());
  }
  SECTION("CapacityIsSameAfterCopy") {
    CacheTest t;
    t.cache.capacity(100);
    auto cache2 = t.cache;

    CHECK(t.cache.capacity() == cache2.capacity());
  }
  SECTION("CapacityIsSameAfterMove") {
    CacheTest t;
    t.cache.capacity(100);
    auto cache2 = std::move(t.cache);

    CHECK(cache2.capacity() == 100);
  }
  SECTION("ComparisonOperatorWorks") {
    CacheTest t;
    REQUIRE(t.cache == t.cache);

    auto cache2 = t.cache;
    CHECK(t.cache == cache2);

    t.cache.emplace("one", 1);
    cache2.emplace("one", 1);
    CHECK(t.cache == cache2);

    t.cache.emplace("two", 2);
    cache2.emplace("two", 2);
    CHECK(t.cache == cache2);

    t.cache.erase("two");
    CHECK(t.cache != cache2);
  }
  SECTION("SwapWorks") {
    CacheTest t;
    auto cache2 = t.cache;

    t.cache.emplace("one", 1);
    cache2.emplace("two", 2);

    REQUIRE(t.cache.contains("one"));
    REQUIRE(cache2.contains("two"));

    t.cache.swap(cache2);

    CHECK_FALSE(t.cache.contains("one"));
    CHECK(t.cache.contains("two"));
    CHECK_FALSE(cache2.contains("two"));
    CHECK(cache2.contains("one"));
  }
  SECTION("SizeStaysZeroWhenCapacityZero") {
    CacheTest t;
    t.cache.capacity(0);

    REQUIRE(t.cache.capacity() == 0);
    REQUIRE(t.cache.size() == 0);

    auto result = t.cache.insert("one", 1);

    CHECK(t.cache.capacity() == 0);
    CHECK(t.cache.size() == 0);
    CHECK_FALSE(result.was_inserted());
    CHECK(result.iterator() == t.cache.end());

    result = t.cache.emplace("two", 2);

    CHECK(t.cache.capacity() == 0);
    CHECK(t.cache.size() == 0);
    CHECK_FALSE(result.was_inserted());
    CHECK(result.iterator() == t.cache.end());
  }
  SECTION("LookupsMoveElementsToFront") {
    CacheTest t;
    t.cache.capacity(2);
    t.cache.insert({{"one", 1}, {"two", 2}});

    // The LRU principle mandates that lookups place
    // accessed elements to the front.
    auto value = t.cache.find("one");
    t.cache.emplace("three", 3);

    CHECK(t.cache.contains("one"));
    CHECK_FALSE(t.cache.contains("two"));
    CHECK(t.cache.contains("three"));
    CHECK(std::prev(t.cache.ordered_cend())->get() == "three");
    CHECK(t.cache.front() == "three");
    CHECK(t.cache.back() == "one");

    REQUIRE(t.cache.lookup("one") == 1);
    CHECK(std::prev(t.cache.ordered_cend())->get() == "one");
    CHECK(t.cache.ordered_cbegin()->get() == "three");
    CHECK(t.cache.front() == "one");
    CHECK(t.cache.back() == "three");
  }
}
