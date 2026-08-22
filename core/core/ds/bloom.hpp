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
#pragma once
#include <array>
#include <bit>
#include <span>
#include <type_traits>
#include "core/ds/hash/splitmix64.hpp"
#include "core/integers.h"
#include "core/macros.hpp"

namespace pepp {

// While multiplicative hashes tend to yield poor results, using 2^64/phi as a multiplicand is suprisingly effective for
// mapping for a large range to a smaller range. In particular, it will distribute consecutive keys very effective. A
// major issue is that the low-order bits are less likely to change, which is why our bloom filter discards them with
// a shift.
// https://probablydance.com/2018/06/16/fibonacci-hashing-the-optimization-that-the-world-forgot-or-a-better-alternative-to-integer-modulo/
// Knuth, TAOCP vol. 3 sec. 6.4.
struct FibonacciHash {
  static constexpr u64 hash(u64 x) noexcept { return x * 0x9e3779b97f4a7c15ull; }
};

// At the cost of a shift + XOR, we can transfer some entropy of the high bits to the low bits.
struct FibonacciMixHash {
  static constexpr u64 hash(u64 x) noexcept {
    x *= 0x9e3779b97f4a7c15ull;
    return x ^ (x >> 32);
  }
};

// Horrid hash algorithm which returns itself. Useful as a control against which we compare.
struct IdentityHash {
  static constexpr u64 hash(u64 x) noexcept { return x; }
};

// Highest quality hash we currently provide wtih ~3x the number of cycles/hash as FibonacciHash.
struct SplitMix64Hash {
  static constexpr u64 hash(u64 x) noexcept { return splitmix64(x); }
};

// A fixed-size split block Bloom filter which rejects all true negatives at the expense of some false positives.
// It's based heavily on Apache Parquet's specification.
// Other useful resources when designing this filter were:
// Apple, "Split block Bloom filters" (arXiv:2101.01719),  https://arxiv.org/pdf/2101.01719
// Lang et al, "Performance-Optimal Filtering", https://dl.acm.org/doi/abs/10.14778/3303753.3303757
//
// Parquet uses 256-bit blocks composed of 32-bit words while we use 64-bit blocks with 64/k-bit words.
// For our most important targets (x84-64, arm64) a block fits nicely in a register, and operations should be
// shifts ands masks oer that register.
//
// Each key sets K bits within its selected block, and each bit set is in a different word within that block.
//
//   BYTES   filter footprint. Any non-zero multiple of 8.
//   K       Number of bits set within a block by a key. Must be 1, 2, 4, or 8.
//   SHIFT   Drop this many low-order bits of the key before hashing.
//   Hash    any type with `static constexpr u64 hash(u64) noexcept`. A weak hash raises the
//           false-positive rate but cannot cause a false negative.
template <typename Key, std::size_t BYTES, unsigned K = 2, unsigned SHIFT = 0, class Hash = FibonacciMixHash>
class SplitBlockBloom {
public:
  using key_type = Key;
  static constexpr std::size_t NBLOCKS = BYTES / 8;
  static constexpr u32 WORDS_PER_BLOCK = K;
  // Multiply-shift keeps the top WORD_BITS of each 32x32->64 bit product.
  static constexpr unsigned WORD_BITS = 64 / K;
  // Verbatim from the Parquet specification; we use the first K.
  static constexpr std::array<u32, 8> SALT = {0x47b6137bu, 0x44974d91u, 0x8824ad5bu, 0xa2b7289du,
                                              0x705495c7u, 0x2df1424bu, 0x9efc4947u, 0x5c6bfb31u};

  static_assert(std::is_unsigned_v<Key>, "Key must be an unsigned integer type");
  static_assert(BYTES >= 8 && BYTES % 8 == 0, "BYTES must be a non-zero multiple of sizeof(u64)");
  static_assert(K >= 1 && K <= 8 && std::has_single_bit(K), "K must be a power of two in [1, 8]");
  static_assert(NBLOCKS <= (u64(1) << 32), "block selection requires NBLOCKS <= 2^32");

  constexpr SplitBlockBloom() = default;
  explicit constexpr SplitBlockBloom(std::span<const Key> keys) { rebuild(keys); }
  // Keep copy+move ctor+assign, but just be warned that they can be quite slow.

  // Removal means rebuilding from scratch
  constexpr void rebuild(std::span<const Key> keys) noexcept {
    clear();
    for (auto key : keys) insert(key);
  }
  constexpr void clear() noexcept { _blocks.fill(0); }
  constexpr void insert(Key key) noexcept {
    const u64 h = Hash::hash(static_cast<u64>(key) >> SHIFT);
    _blocks[block_index(h)] |= mask(static_cast<u32>(h));
  }

  // False means definitely absent, true requires checking authoritative source.
  [[nodiscard]] PEPP_ALWAYS_INLINE constexpr bool maybe_contains(Key key) const noexcept {
    const u64 h = Hash::hash(static_cast<u64>(key) >> SHIFT);
    const u64 masked = mask(static_cast<u32>(h));
    return (_blocks[block_index(h)] & masked) == masked;
  }

  // Count the number of set bits in the filter. False-positive rate correlates to fullness, meaning I need to measure
  // that rate to tune filter instantiations.
  [[nodiscard]] constexpr std::size_t popcount() const noexcept {
    std::size_t count = 0;
    for (auto word : _blocks) count += static_cast<std::size_t>(std::popcount(word));
    return count;
  }

private:
  // One bit per word, so a key sets exactly K bits. Two of its own bits can never collide.
  static constexpr u64 mask(u32 x) noexcept {
    // Select the top bits of the product.
    static constexpr unsigned PROD_SHIFT = 32 - static_cast<unsigned>(std::countr_zero(WORD_BITS));
    u64 result = 0;
    for (unsigned i = 0; i < K; ++i) {
      u32 y = x * SALT[i];
      result |= 1ULL << (i * WORD_BITS + (y >> PROD_SHIFT));
    }
    return result;
  }
  // Convert u64 to [0, NBLOCKS) while avoiding integer division and power-of-two constraints.
  static constexpr std::size_t block_index(u64 h) noexcept {
    // We can avoid a 128-bit multiply first truncating our u64 to only have 32 significant bits. NBLOCKs is <=2^32,
    // so we really have two u32s expressed as u64s. Multiply them together and extract the high order bits (which is
    // Lemire's trick).
    return static_cast<std::size_t>(((h >> 32) * NBLOCKS) >> 32);
  }
  std::array<u64, NBLOCKS> _blocks = {};
};
} // namespace pepp
