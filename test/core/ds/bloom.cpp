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
#include "core/ds/bloom.hpp"
#include <catch.hpp>
#include <limits>
#include <span>
#include <vector>

using namespace pepp;

namespace {

// Generate a number of keys for testing using PRNG for reproducibility.
std::vector<u32> make_keys(std::size_t count, u64 seed = 0x1234'5678'9abc'def0ull) {
  std::vector<u32> keys;
  keys.reserve(count);
  u64 state = seed;
  for (std::size_t i = 0; i < count; ++i) {
    state = splitmix64(state);
    keys.push_back(static_cast<u32>(state));
  }
  return keys;
}

template <class Filter> void check_no_false_negatives(std::span<const typename Filter::key_type> keys) {
  Filter filter;
  filter.rebuild(keys);
  for (auto key : keys) CHECK(filter.maybe_contains(key));
}

} // namespace

TEST_CASE("SplitBlockBloom", "[scope:core][kind:unit][arch:*]") {
  using Key = u32;
  const auto members = make_keys(24);
  const auto keys = std::span<const Key>(members);

  SECTION("An empty filter rejects everything") {
    SplitBlockBloom<Key, 256> filter;
    CHECK(filter.popcount() == 0);
    for (auto key : make_keys(4096, 0xfeed'face'dead'beefull)) CHECK_FALSE(filter.maybe_contains(key));
  }

  SECTION("No false negatives, across sizes and hash algorithms") {
    check_no_false_negatives<SplitBlockBloom<Key, 64, 1>>(keys);
    check_no_false_negatives<SplitBlockBloom<Key, 64, 2>>(keys);
    check_no_false_negatives<SplitBlockBloom<Key, 256, 2>>(keys);
    check_no_false_negatives<SplitBlockBloom<Key, 1024, 2>>(keys);
    // Increase K.
    check_no_false_negatives<SplitBlockBloom<Key, 256, 4>>(keys);
    check_no_false_negatives<SplitBlockBloom<Key, 256, 8>>(keys);
    // Dropping the low bits won't affect correctness, just false-positive rate.
    check_no_false_negatives<SplitBlockBloom<Key, 256, 2, 2>>(keys);
    check_no_false_negatives<SplitBlockBloom<Key, 256, 2, 6>>(keys);
    // Footprints that are not a power of two.
    check_no_false_negatives<SplitBlockBloom<Key, 24, 2>>(keys);
    check_no_false_negatives<SplitBlockBloom<Key, 1000, 4, 2>>(keys);
    // Test all hash policies
    check_no_false_negatives<SplitBlockBloom<Key, 256, 2, 0, SplitMix64Hash>>(keys);
    check_no_false_negatives<SplitBlockBloom<Key, 256, 2, 0, FibonacciHash>>(keys);
    check_no_false_negatives<SplitBlockBloom<Key, 256, 2, 0, IdentityHash>>(keys);
  }

  SECTION("Overfilling degrades the false-positive rate but not correctness") {
    // Ineffective filter, but ever key will return true.
    const auto many = make_keys(4096);
    check_no_false_negatives<SplitBlockBloom<Key, 64, 2>>(many);
  }

  SECTION("clear() returns the filter to empty") {
    SplitBlockBloom<Key, 256> filter(keys);
    CHECK(filter.popcount() > 0);
    filter.clear();
    CHECK(filter.popcount() == 0);
    CHECK_FALSE(filter.maybe_contains(members.front()));
  }

  SECTION("rebuild() functions as a clear") {
    SplitBlockBloom<Key, 1024, 2> filter(keys);
    CHECK(filter.popcount() > 0);
    filter.rebuild(std::span<const Key>{});
    CHECK(filter.popcount() == 0);
  }

  SECTION("SHIFT quantizes keys") {
    constexpr unsigned kShift = 4;
    SplitBlockBloom<Key, 256, 2, kShift> filter;
    const Key key = 0x0001'2340;
    filter.insert(key);
    for (Key offset = 0; offset < (1u << kShift); ++offset) CHECK(filter.maybe_contains(key + offset));
  }

  SECTION("Footprints are as expected") {
    CHECK(sizeof(SplitBlockBloom<Key, 64>) == 64);
    CHECK(sizeof(SplitBlockBloom<Key, 256>) == 256);
    CHECK(sizeof(SplitBlockBloom<Key, 1024, 2>) == 1024);
    CHECK(sizeof(SplitBlockBloom<Key, 24, 2>) == 24);
    CHECK(SplitBlockBloom<Key, 256>::NBLOCKS == 32);
  }

  SECTION("Usable at compile time") {
    constexpr bool found = [] {
      SplitBlockBloom<Key, 64, 2> filter;
      filter.insert(0x1234'5678);
      return filter.maybe_contains(0x1234'5678);
    }();
    STATIC_REQUIRE(found);
  }
}
