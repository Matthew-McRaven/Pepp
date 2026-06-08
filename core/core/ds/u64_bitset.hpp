#pragma once
#include "core/integers.h"
namespace pepp {
// A poor-mans reimplementation of std::bitset.
// It exists because I want "fast" zero/one counting with popcount, which is not supported by std::bitset.
// The downside of this bitset is that it cannot store more than 64 elements.
template <size_t COUNT> class FixedBitset {
public:
  static_assert(COUNT <= 64, "COUNT must be <= 64 because this class uses u64 as storage");

  class Reference {
  public:
    constexpr Reference(FixedBitset &bitset, size_t pos) : _bitset(bitset), _pos(pos) {}
    constexpr operator bool() const { return _bitset._storage & (1ULL << _pos); }
    constexpr bool operator~() const { return !(*this); }
    constexpr Reference &operator=(const Reference &other) {
      bool value = static_cast<bool>(other);
      return *this = value;
    }
    constexpr Reference &operator=(bool value) {
      if (!value) _bitset.clear_bit(_pos);
      else _bitset.enable_bit(_pos);
      return *this;
    }
    constexpr Reference &flip() {
      _bitset.flip(_pos);
      return *this;
    }

  private:
    FixedBitset &_bitset;
    size_t _pos;
  };
  using bitset = FixedBitset<COUNT>;
  using reference = Reference;
  constexpr FixedBitset() = default;
  constexpr FixedBitset(const FixedBitset &) = default;
  constexpr FixedBitset &operator=(const FixedBitset &) = default;
  constexpr FixedBitset(FixedBitset &&) = default;
  constexpr FixedBitset &operator=(FixedBitset &&) = default;
  constexpr FixedBitset(u64 value) : _storage(value & MASK) {}
  constexpr FixedBitset &operator=(u64 value) {
    _storage = value;
    return *this;
  }
  static constexpr bitset ones() { return bitset(MASK); }
  static constexpr bitset zeros() { return bitset(0); }

  // bitset operations, per  23.3.5.2:
  constexpr bitset &operator&=(const bitset &rhs) noexcept {
    _storage &= rhs._storage & MASK;
    return *this;
  }
  constexpr bitset &operator|=(const bitset &rhs) noexcept {
    _storage |= rhs._storage & MASK;
    return *this;
  }
  constexpr bitset &operator^=(const bitset &rhs) noexcept {
    _storage ^= rhs._storage & MASK;
    return *this;
  }
  constexpr bitset &operator<<=(size_t pos) noexcept {
    _storage <<= pos;
    _storage &= MASK;
    return *this;
  }
  constexpr bitset &operator>>=(size_t pos) noexcept {
    _storage >>= pos;
    _storage &= MASK;
    return this;
  }
  constexpr bitset &set() noexcept {
    _storage = MASK;
    return *this;
  }
  // I don't like this overload because it introduces a data-dependent branch which is hard to predict accurately.
  // I'll include the source code for its implementation, but profiling indicates it is a bad idea to use.
  /*constexpr bitset &set(size_t pos, bool val = true) {
    if (val) _storage |= ((1ULL << pos)) & MASK;
    else _storage &= ~(1ULL << pos) & MASK;
    return *this;
  }*/
  constexpr bitset &reset() noexcept {
    _storage = 0;
    return *this;
  }
  constexpr bitset &reset(size_t pos) {
    _storage &= ~(1ULL << pos) & MASK;
    return *this;
  }
  constexpr bitset operator~() const noexcept { return bitset(~_storage & MASK); }
  constexpr bitset &flip() noexcept {
    _storage = ~_storage & MASK;
    return *this;
  }
  constexpr bitset &flip(size_t pos) {
    _storage ^= (1ULL << pos) & MASK;
    return *this;
  }

  constexpr bool operator[](size_t pos) const { return (_storage >> pos) & 1; }
  constexpr reference operator[](size_t pos) { return Reference(*this, pos); }
  constexpr u64 to_u64() const { return _storage; }
  constexpr size_t size() const noexcept { return COUNT; }
  constexpr bool operator==(const FixedBitset &other) const noexcept { return _storage == other._storage; }
  constexpr bool test(size_t pos) const { return (_storage >> pos) & 1; }
  constexpr bool all() const noexcept { return count() == COUNT; }
  constexpr bool any() const noexcept { return _storage != 0; }
  constexpr bool none() const noexcept { return _storage == 0; }

  // Matthew's extensions
  constexpr u8 countr_zero() const { return std::countr_zero(_storage); }
  constexpr u8 countr_one() const { return std::countr_one(_storage); }
  constexpr u8 count() const { return std::popcount(_storage); }
  constexpr void clear() { _storage = 0; }
  // Does not have partiy w/ clear/toggle because I do not have a typical SET(pos) overload.
  constexpr bitset &enable_bit(size_t pos) noexcept {
    _storage |= ((1ULL << pos)) & MASK;
    return *this;
  }
  constexpr bitset &clear_bit(size_t pos) noexcept { return reset(pos); }
  constexpr bitset &toggle_bit(size_t pos) noexcept { return flip(pos); }
  constexpr u64 operator()() const { return _storage; }

private:
  using storage_t =
      std::conditional_t<(COUNT <= 8), u8,
                         std::conditional_t<(COUNT <= 16), u16, std::conditional_t<(COUNT <= 32), u32, u64>>>;
  storage_t _storage = 0;
  static constexpr storage_t MASK_generator() noexcept {
    if constexpr (sizeof(storage_t) * 8 == COUNT) return ~storage_t(0);
    else return (storage_t(1) << COUNT) - 1;
  }
  static constexpr storage_t MASK = MASK_generator();
};

template <size_t N> FixedBitset<N> operator&(const FixedBitset<N> &lhs, const FixedBitset<N> &rhs) noexcept {
  return FixedBitset<N>(lhs.to_u64() & rhs.to_u64());
}

template <size_t N> FixedBitset<N> operator|(const FixedBitset<N> &lhs, const FixedBitset<N> &rhs) noexcept {
  return FixedBitset<N>(lhs.to_u64() | rhs.to_u64());
}

template <size_t N> FixedBitset<N> operator^(const FixedBitset<N> &lhs, FixedBitset<N> &rhs) noexcept {
  return FixedBitset<N>(lhs.to_u64() ^ rhs.to_u64());
}
} // namespace pepp