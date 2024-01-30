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
using UnorderedIterator = typename CacheType::UnorderedIterator;
using UnorderedConstIterator = typename CacheType::UnorderedConstIterator;
using OrderedIterator = typename CacheType::OrderedIterator;
using OrderedConstIterator = typename CacheType::OrderedConstIterator;
CacheType cache;

TEST_CASE("IteratorTest") {
  SECTION("UnorderedIteratorsAreCompatibleAsExpected") {
    CacheType cache;
    cache.emplace("one", 1);

    // Move construction
    UnorderedIterator first(cache.unordered_begin());

    // Copy construction
    UnorderedIterator second(first);

    // Copy assignment
    UnorderedIterator third;
    third = second;

    // Move construction from non-const to const
    UnorderedConstIterator first_const(std::move(first));

    // Copy construction from non-const to const
    UnorderedConstIterator second_const(second);

    // Copy assignment
    UnorderedConstIterator third_const;
    third_const = third;
  }
  SECTION("OrderedIteratorsAreCompatibleAsExpected") {
    CacheType cache;
    cache.emplace("one", 1);

    // Move construction
    OrderedIterator first(cache.ordered_begin());

    // TODO: Copy CTOR is broken on const-ness
    // Copy construction
    // OrderedIterator second(first);

    // Copy assignment
    OrderedIterator third;
    third = first;

    // Move construction from non-const to const
    OrderedConstIterator first_const(std::move(first));

    // Copy construction from non-const to const
    // OrderedConstIterator second_const(third);

    // TODO: Copy assigment is broken on const-ness
    // Copy assignment
    // OrderedConstIterator third_const;
    // third_const = first_const;
  }
  SECTION("OrderedAndUnorderedAreComparable") {
    CacheType cache;
    cache.emplace("one", 1);

    // Basic assumptions
    REQUIRE(cache.unordered_begin() == cache.unordered_begin());
    REQUIRE(cache.ordered_begin() == cache.ordered_begin());
    REQUIRE(cache.unordered_end() == cache.unordered_end());
    REQUIRE(cache.ordered_end() == cache.ordered_end());

    CHECK(cache.unordered_begin() == cache.ordered_begin());

    // We need to ensure symmetry!
    CHECK(cache.ordered_begin() == cache.unordered_begin());

    // This is an exceptional property we expect
    CHECK(cache.unordered_end() == cache.ordered_end());
    CHECK(cache.ordered_end() == cache.unordered_end());

    // These assumptions should hold because there is only one element
    // so the unordered iterator will convert to an ordered iterator, then
    // compare equal because both point to the same single element.
    CHECK(cache.ordered_begin() == cache.unordered_begin());
    CHECK(cache.unordered_begin() == cache.ordered_begin());

    cache.emplace("two", 1);

    // But then the usual assumptions should hold
    CHECK(cache.ordered_begin() != cache.find("two"));
    CHECK(cache.find("two") != cache.ordered_begin());
  }
  SECTION("TestConversionFromUnorderedToOrdered") {
    CacheType cache;
    cache.emplace("one", 1);
    cache.emplace("two", 2);
    cache.emplace("three", 3);

    // Note: find() will always return end() - 1
    UnorderedIterator unordered = cache.find("one");

    REQUIRE(unordered.key() == "one");
    REQUIRE(unordered.value() == 1);

    // TODO: Broken const-conversion
    // OrderedIterator ordered(unordered);
    /*OrderedIterator ordered(unordered);

    CHECK(ordered.key() == "one");
    CHECK(ordered.value() == 1);

    // Once it's ordered, the ordering shold be maintained
    --ordered;
    CHECK(ordered.key() == "three");
    CHECK(ordered.value() == 3);

    UnorderedConstIterator const_unordered = unordered;
    const_unordered = unordered;

    OrderedConstIterator const_ordered(std::move(const_unordered));
    const_ordered = OrderedConstIterator(std::move(const_unordered));

    // Just making sure this compiles
    const_ordered = --ordered;
    const_ordered = OrderedConstIterator(unordered);

    CHECK(ordered.key() == "two");
    CHECK(ordered.value() == 2);*/
  }
  SECTION("OrdereredIteratorsAreOrdered") {
    CacheType cache;
    for (std::size_t i = 0; i < 100; ++i) {
      cache.emplace(std::to_string(i), i);
    }

    auto iterator = cache.ordered_begin();
    for (std::size_t i = 0; i < 100; ++i, ++iterator) {
      REQUIRE(iterator.value() == i);
    }
  }
  SECTION("OrderedIteratorsDoNotChangeTheOrderOfElements") {
    CacheType cache;
    cache.capacity(2);
    cache.insert({{"one", 1}});

    auto begin = cache.ordered_begin();

    cache.emplace("two", 2);

    // This here will cause a lookup, but it should not
    // change the order of elements
    REQUIRE(begin->key() == "one");
    REQUIRE((++begin)->key() == "two");
    REQUIRE((--begin)->key() == "one");
    cache.emplace("three", 3);

    CHECK_FALSE(cache.contains("one"));
    CHECK(cache.contains("two"));
    CHECK(cache.contains("three"));
  }
  SECTION("UnorderedIteratorsDoNotChangeTheOrderOfElements") {
    CacheType cache;
    cache.capacity(2);
    cache.insert({{"one", 1}});

    auto begin = cache.unordered_begin();

    cache.emplace("two", 2);

    REQUIRE(begin->key() == "one");
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
    /*
    auto ordered_iterator = cache.ordered_begin();
    CHECK(cache.is_valid(ordered_iterator));
    CHECK(cache.is_valid(++ordered_iterator));
    */

    auto unordered_iterator = cache.unordered_begin();
    CHECK(cache.is_valid(unordered_iterator));
    CHECK(cache.is_valid(++unordered_iterator));
  }
  SECTION("ThrowIfInvalidThrowsAsExpected") {
    CacheType cache;
    CHECK_THROWS_AS(cache.throw_if_invalid(cache.ordered_begin()),
                    LRU::Error::InvalidIterator);
    CHECK_THROWS_AS(cache.throw_if_invalid(cache.ordered_end()),
                    LRU::Error::InvalidIterator);
    CHECK_THROWS_AS(cache.throw_if_invalid(cache.unordered_begin()),
                    LRU::Error::InvalidIterator);
    CHECK_THROWS_AS(cache.throw_if_invalid(cache.unordered_end()),
                    LRU::Error::InvalidIterator);
  }
  SECTION("ThrowIfInvalidThrowsAsExpected") {
    CacheType cache;
    CHECK_THROWS_AS(cache.throw_if_invalid(cache.ordered_begin()),
                    LRU::Error::InvalidIterator);
    CHECK_THROWS_AS(cache.throw_if_invalid(cache.ordered_end()),
                    LRU::Error::InvalidIterator);
    CHECK_THROWS_AS(cache.throw_if_invalid(cache.unordered_begin()),
                    LRU::Error::InvalidIterator);
    CHECK_THROWS_AS(cache.throw_if_invalid(cache.unordered_end()),
                    LRU::Error::InvalidIterator);
  }
  SECTION("IsValidReturnsFalseForInvalidIterators") {
    CacheType cache;
    CHECK_FALSE(cache.is_valid(cache.ordered_begin()));
    CHECK_FALSE(cache.is_valid(cache.ordered_end()));
    CHECK_FALSE(cache.is_valid(cache.unordered_begin()));
    CHECK_FALSE(cache.is_valid(cache.unordered_end()));
  }
}
