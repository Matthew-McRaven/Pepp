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
#include <memory>
#include <vector>

#include "lru/internal/statistics-mutator.hpp"
#include "lru/lru.hpp"

using namespace LRU;
using namespace LRU::Internal;
struct CacheWithStatisticsTest {
  void assert_total_stats(int accesses, int hits, int misses) {
    REQUIRE(cache.stats().total_accesses() == accesses);
    REQUIRE(cache.stats().total_hits() == hits);
    REQUIRE(cache.stats().total_misses() == misses);
  }

  void expect_total_stats(int accesses, int hits, int misses) {
    CHECK(cache.stats().total_accesses() == accesses);
    CHECK(cache.stats().total_hits() == hits);
    CHECK(cache.stats().total_misses() == misses);
  }

  Cache<int, int> cache;
};
TEST_CASE("StatisticsTest") {
  SECTION("ConstructsWellFromRange") {
    std::vector<int> range = {1, 2, 3};
    Statistics<int> stats(range);

    for (const auto& i : range) {
      REQUIRE(stats.is_monitoring(i));
    }
  };
  SECTION("ConstructsWellFromIterator") {
    std::vector<int> range = {1, 2, 3};
    Statistics<int> stats(range.begin(), range.end());

    for (const auto& i : range) {
      REQUIRE(stats.is_monitoring(i));
    }
  };
  SECTION("ConstructsWellFromInitializerList") {
    Statistics<int> stats({1, 2, 3});

    std::vector<int> range = {1, 2, 3};
    for (const auto& i : range) {
      REQUIRE(stats.is_monitoring(i));
    }
  };
  SECTION("ConstructsWellFromVariadicArguments") {
    Statistics<int> stats(1, 2, 3);

    std::vector<int> range = {1, 2, 3};
    for (const auto& i : range) {
      REQUIRE(stats.is_monitoring(i));
    }
  };
  SECTION("EmptyPreconditions") {
    Statistics<int> stats;

    CHECK_FALSE(stats.is_monitoring_keys());
    CHECK(stats.number_of_monitored_keys() == 0);
    CHECK_FALSE(stats.is_monitoring(1));
    CHECK_FALSE(stats.is_monitoring(2));
    CHECK(stats.total_accesses() == 0);
    CHECK(stats.total_hits() == 0);
    CHECK(stats.total_misses() == 0);
  };
  SECTION("StatisticsMutatorCanRegisterHits") {
    auto stats = std::make_shared<Statistics<int>>(1, 2, 3);
    StatisticsMutator<int> mutator(stats);

    mutator.register_hit(1);
    CHECK(stats->hits_for(1) == 1);
    CHECK(stats->total_accesses() == 1);
    CHECK(stats->total_hits() == 1);
    CHECK(stats->total_misses() == 0);
    CHECK(stats->hit_rate() == 1);
    CHECK(stats->miss_rate() == 0);

    mutator.register_hit(1);
    CHECK(stats->hits_for(1) == 2);
    CHECK(stats->total_accesses() == 2);
    CHECK(stats->total_hits() == 2);
    CHECK(stats->total_misses() == 0);
    CHECK(stats->hit_rate() == 1);
    CHECK(stats->miss_rate() == 0);

    mutator.register_hit(2);
    CHECK(stats->hits_for(1) == 2);
    CHECK(stats->hits_for(2) == 1);
    CHECK(stats->total_accesses() == 3);
    CHECK(stats->total_hits() == 3);
    CHECK(stats->total_misses() == 0);
    CHECK(stats->hit_rate() == 1);
    CHECK(stats->miss_rate() == 0);
  };
  SECTION("StatisticsMutatorCanRegisterMisses") {
    auto stats = std::make_shared<Statistics<int>>(1, 2, 3);
    StatisticsMutator<int> mutator(stats);

    mutator.register_miss(1);
    CHECK(stats->misses_for(1) == 1);
    CHECK(stats->total_accesses() == 1);
    CHECK(stats->total_hits() == 0);
    CHECK(stats->total_misses() == 1);
    CHECK(stats->hit_rate() == 0);
    CHECK(stats->miss_rate() == 1);

    mutator.register_miss(1);
    CHECK(stats->misses_for(1) == 2);
    CHECK(stats->total_accesses() == 2);
    CHECK(stats->total_hits() == 0);
    CHECK(stats->total_misses() == 2);
    CHECK(stats->hit_rate() == 0);
    CHECK(stats->miss_rate() == 1);

    mutator.register_miss(2);
    CHECK(stats->misses_for(1) == 2);
    CHECK(stats->misses_for(2) == 1);
    CHECK(stats->total_accesses() == 3);
    CHECK(stats->total_hits() == 0);
    CHECK(stats->total_misses() == 3);
    CHECK(stats->hit_rate() == 0);
    CHECK(stats->miss_rate() == 1);
  };
  SECTION("CanDynamicallyMonitorAndUnmonitorKeys") {
    Statistics<int> stats;

    REQUIRE(stats.number_of_monitored_keys() == 0);

    stats.monitor(1);

    CHECK(stats.number_of_monitored_keys() == 1);
    CHECK(stats.is_monitoring(1));
    CHECK_FALSE(stats.is_monitoring(2));

    stats.monitor(2);

    CHECK(stats.number_of_monitored_keys() == 2);
    CHECK(stats.is_monitoring(1));
    CHECK(stats.is_monitoring(2));

    stats.unmonitor(1);

    CHECK(stats.number_of_monitored_keys() == 1);
    CHECK_FALSE(stats.is_monitoring(1));
    CHECK(stats.is_monitoring(2));

    stats.unmonitor_all();

    CHECK_FALSE(stats.is_monitoring_keys());
    CHECK_FALSE(stats.is_monitoring(1));
    CHECK_FALSE(stats.is_monitoring(2));
  };
  SECTION("ThrowsForUnmonitoredKey") {
    Statistics<int> stats;

    CHECK_THROWS_AS(stats.stats_for(1), LRU::Error::UnmonitoredKey);
    CHECK_THROWS_AS(stats.hits_for(2), LRU::Error::UnmonitoredKey);
    CHECK_THROWS_AS(stats.misses_for(3), LRU::Error::UnmonitoredKey);
    CHECK_THROWS_AS(stats[4], LRU::Error::UnmonitoredKey);
  };
  SECTION("RatesAreCalculatedCorrectly") {
    auto stats = std::make_shared<Statistics<int>>(1, 2, 3);
    StatisticsMutator<int> mutator(stats);

    for (std::size_t i = 0; i < 20; ++i) {
      mutator.register_hit(1);
    }

    for (std::size_t i = 0; i < 80; ++i) {
      mutator.register_miss(1);
    }

    CHECK(stats->hit_rate() == 0.2);
    CHECK(stats->miss_rate() == 0.8);
  };
  SECTION("CanShareStatistics") {
    auto stats = std::make_shared<Statistics<int>>(1, 2, 3);
    StatisticsMutator<int> mutator1(stats);
    StatisticsMutator<int> mutator2(stats);
    StatisticsMutator<int> mutator3(stats);

    REQUIRE(mutator1.shared() == mutator2.shared());
    REQUIRE(mutator2.shared() == mutator3.shared());
    REQUIRE(&mutator2.get() == &mutator3.get());

    mutator1.register_hit(1);
    CHECK(stats->total_accesses() == 1);
    CHECK(stats->total_hits() == 1);
    CHECK(stats->total_misses() == 0);
    CHECK(stats->hits_for(1) == 1);

    mutator2.register_hit(1);
    CHECK(stats->total_accesses() == 2);
    CHECK(stats->total_hits() == 2);
    CHECK(stats->total_misses() == 0);
    CHECK(stats->hits_for(1) == 2);

    mutator3.register_miss(2);
    CHECK(stats->total_accesses() == 3);
    CHECK(stats->total_hits() == 2);
    CHECK(stats->total_misses() == 1);
    CHECK(stats->hits_for(1) == 2);
    CHECK(stats->misses_for(1) == 0);
    CHECK(stats->hits_for(2) == 0);
    CHECK(stats->misses_for(2) == 1);
  };
  SECTION("RequestForCacheStatisticsThrowsWhenNoneRegistered") {
    CacheWithStatisticsTest t;
    CHECK_THROWS_AS(t.cache.stats(), LRU::Error::NotMonitoring);
  };
  SECTION("CanRegisterLValueStatistics") {
    CacheWithStatisticsTest t;
    auto stats = std::make_shared<Statistics<int>>();
    t.cache.monitor(stats);

    CHECK(t.cache.is_monitoring());

    // This is a strong constraint, but must hold for lvalue stats object
    CHECK(&t.cache.stats() == &*stats);

    t.cache.contains(1);
    CHECK(t.cache.shared_stats()->total_accesses() == 1);
    CHECK(t.cache.stats().total_misses() == 1);

    t.cache.emplace(1, 2);

    t.cache.contains(1);
    CHECK(t.cache.stats().total_accesses() == 2);
    CHECK(t.cache.stats().total_misses() == 1);
    CHECK(t.cache.stats().total_hits() == 1);
  };
  SECTION("CanRegisterRValueStatistics") {
    CacheWithStatisticsTest t;
    auto s = std::make_unique<Statistics<int>>(1);
    t.cache.monitor(std::move(s));

    CHECK(t.cache.is_monitoring());

    t.cache.contains(1);
    CHECK(t.cache.stats().total_accesses() == 1);
    CHECK(t.cache.stats().total_misses() == 1);

    t.cache.emplace(1, 2);

    t.cache.contains(1);
    CHECK(t.cache.stats().total_accesses() == 2);
    CHECK(t.cache.stats().total_misses() == 1);
    CHECK(t.cache.stats().total_hits() == 1);
  };
  SECTION("CanConstructItsOwnStatistics") {
    CacheWithStatisticsTest t;
    t.cache.monitor(1, 2, 3);

    CHECK(t.cache.is_monitoring());
    CHECK(t.cache.stats().is_monitoring(1));
    CHECK(t.cache.stats().is_monitoring(2));
    CHECK(t.cache.stats().is_monitoring(3));

    t.cache.contains(1);
    CHECK(t.cache.stats().total_accesses() == 1);
    CHECK(t.cache.stats().total_misses() == 1);

    t.cache.emplace(1, 2);

    t.cache.contains(1);
    CHECK(t.cache.stats().total_accesses() == 2);
    CHECK(t.cache.stats().total_misses() == 1);
    CHECK(t.cache.stats().total_hits() == 1);
  };
  SECTION("KnowsWhenItIsMonitoring") {
    CacheWithStatisticsTest t;
    CHECK_FALSE(t.cache.is_monitoring());

    t.cache.monitor();

    CHECK(t.cache.is_monitoring());

    t.cache.stop_monitoring();

    CHECK_FALSE(t.cache.is_monitoring());
  };
  SECTION("StatisticsWorkWithCache") {
    CacheWithStatisticsTest t;
    t.cache.monitor(1);
    REQUIRE(t.cache.is_monitoring());
    t.assert_total_stats(0, 0, 0);

    // contains
    t.cache.contains(1);
    t.expect_total_stats(1, 0, 1);

    // An access should only occur for lookup(),
    // find(), contains() and operator[]
    t.cache.emplace(1, 1);
    t.expect_total_stats(1, 0, 1);

    t.cache.contains(1);
    t.expect_total_stats(2, 1, 1);

    // find
    t.cache.find(2);
    t.expect_total_stats(3, 1, 2);

    t.cache.emplace(2, 2);

    t.cache.find(2);
    t.expect_total_stats(4, 2, 2);

    CHECK_THROWS_AS(t.cache.lookup(3), LRU::Error::KeyNotFound);
    t.expect_total_stats(5, 2, 3);

    t.cache.emplace(3, 3);

    REQUIRE(t.cache.lookup(3) == 3);
    t.expect_total_stats(6, 3, 3);

    CHECK_THROWS_AS(t.cache[4], LRU::Error::KeyNotFound);
    t.expect_total_stats(7, 3, 4);

    t.cache.emplace(4, 4);

    REQUIRE(t.cache[4] == 4);
    t.expect_total_stats(8, 4, 4);
  };
  SECTION("StopsMonitoringWhenAsked") {
    CacheWithStatisticsTest t;
    auto stats = std::make_shared<Statistics<int>>(1);
    t.cache.monitor(stats);
    t.cache.emplace(1, 1);

    REQUIRE(t.cache.contains(1));
    REQUIRE(t.cache.stats().hits_for(1) == 1);

    t.cache.stop_monitoring();

    REQUIRE(t.cache.contains(1));
    CHECK(stats->hits_for(1) == 1);
  };
}
