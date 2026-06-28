#pragma once
#include "catch.hpp"
#include "core/integers.h"

inline void compare(const u8 *lhs, const u8 *rhs, u8 length) {
  if (lhs == nullptr || rhs == nullptr) return;
  for (int it = 0; it < length; it++) CHECK(lhs[it] == rhs[it]);
};