#pragma once
#include "catch.hpp"
#include "core/integers.h"
#include "core/sim/api/memory.hpp"

inline void compare(const u8 *lhs, const u8 *rhs, u8 length) {
  if (lhs == nullptr || rhs == nullptr) return;
  for (int it = 0; it < length; it++) CHECK(lhs[it] == rhs[it]);
};

using Changes = std::vector<AddressSpan>;
template <typename T> Changes changes_of(const T &dev) {
  pepp::core::IntervalSet<Address> set;
  dev.collect_changes(set);
  return set.intervals();
}

// Writes `length` bytes of arbitrary-but-deterministic data at `address`.
template <typename T> void poke(T &dev, Address address, std::size_t length, Operation op) {
  static const u8 buf[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
  REQUIRE(length <= sizeof(buf));
  dev.write(address, {buf, length}, op);
}