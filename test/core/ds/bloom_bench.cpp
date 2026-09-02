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
// Measure false-positive rates for SplitBlockBloomFilter.
#include <array>
#include <catch.hpp>
#include <set>
#include <span>
#include <string_view>
#include <unordered_set>
#include <vector>
#include "core/ds/bloom.hpp"
#include "fmt/format.h"

using namespace pepp;

namespace {
// Try keys from 0..kSpace in the test function,
constexpr u32 kSpace = 1u << 16;
// Averaged over this many independently generated member sets.
constexpr unsigned kTrials = 8;
constexpr u64 kSeed = 0x5eed'1234'abcd'0001ull;
constexpr std::array<u32, 3> kCounts = {4, 16, 64};

// Sample count values from from [0, kSpace), in runs of adjacent keys.
// Large run values cluster keys, while small values distribute them.
template <typename Key> std::vector<Key> make_members(u32 count, unsigned run, u64 seed) {
  std::set<Key> keys;
  u64 state = seed;
  while (keys.size() < count) {
    state = splitmix64(state);
    const u32 base = static_cast<u32>(state % kSpace);
    for (unsigned i = 0; i < run && keys.size() < count; ++i) keys.insert(static_cast<Key>((base + i) % kSpace));
  }
  return {keys.begin(), keys.end()};
}

// A result from a single trial.
struct Cell {
  double fp = 0.0;
  double bits = 0.0;
};

// Search the entire declared key space.
template <class Filter> Cell measure(u32 count, unsigned run) {
  using Key = typename Filter::key_type;
  Cell total;
  for (unsigned trial = 0; trial < kTrials; ++trial) {
    const auto members = make_members<Key>(count, run, kSeed + trial);
    const std::unordered_set<Key> truth(members.begin(), members.end());
    const Filter filter(members);
    u64 positives = 0, negatives = 0;
    for (u32 q = 0; q < kSpace; ++q) {
      const Key key = static_cast<Key>(q);
      if (truth.contains(key)) continue;
      negatives += 1, positives += filter.maybe_contains(key);
    }
    total.fp += negatives ? 100.0 * static_cast<double>(positives) / static_cast<double>(negatives) : 0.0;
    total.bits += static_cast<double>(filter.popcount());
  }
  total.fp /= kTrials;
  total.bits /= kTrials;
  return total;
}

// Two row header. Key counts on top, the K values underneath each count.
void print_header(std::string_view title, std::span<const u32> counts, std::span<const unsigned> ks) {
  fmt::print("\n{}\n", title);
  fmt::print("  {:<12}", "");
  for (u32 n : counts) fmt::print("{:^{}}", fmt::format("n={}", n), 8 * ks.size());
  fmt::print("\n  {:<12}", "size");
  for (std::size_t i = 0; i < counts.size(); ++i)
    for (unsigned k : ks) fmt::print("{:>8}", fmt::format("K={}", k));
  fmt::print("\n");
}

template <typename Key, class Hash, std::size_t BYTES, unsigned... K>
void print_size_row(std::span<const u32> counts, std::integer_sequence<unsigned, K...>) {
  static constexpr u32 run_size = 1;
  std::vector<Cell> cells;
  for (u32 n : counts) (cells.push_back(measure<SplitBlockBloom<Key, BYTES, K, 0, Hash>>(n, run_size)), ...);
  fmt::print("  {:<7}{:<5}", fmt::format("{}B", BYTES), "fp%");
  for (const auto &cell : cells) fmt::print("{:>8.2f}", cell.fp);
  fmt::print("\n  {:<7}{:<5}", "", "bits");
  for (const auto &cell : cells) fmt::print("{:>8.0f}", cell.bits);
  fmt::print("\n");
}

template <typename Key, class Hash, std::size_t... BYTES>
void size_table(std::string_view hash_name, std::index_sequence<BYTES...>) {
  constexpr std::array<unsigned, 4> ks = {1, 2, 4, 8};
  print_header(fmt::format("Error Rate for key count and k vs size  shift=0, hash={}", hash_name), kCounts, ks);
  (print_size_row<Key, Hash, BYTES>(kCounts, std::integer_sequence<unsigned, 1, 2, 4, 8>{}), ...);
}
} // namespace

TEST_CASE("SplitBlockBloom false-positive rates", "[scope:core][kind:perf][arch:*]") {
  using Key = u32;
  constexpr auto sizes = std::index_sequence<16, 32, 64, 128, 192, 256, 512, 1024>{};

  size_table<Key, FibonacciHash>("FibonacciHash", sizes);
  size_table<Key, IdentityHash>("IdentityHash", sizes);
  size_table<Key, SplitMix64Hash>("SplitMix64Hash", sizes);
}
