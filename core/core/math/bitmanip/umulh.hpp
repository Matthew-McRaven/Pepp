#pragma once
#include "core/integers.h"
#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_ARM64))
#include <intrin.h>
#endif

namespace bits {

constexpr u64 umulh128h_cpp(u64 lhs, u64 rhs) {
  // Compute the 32-bit parts of the product, which always fit in a 64-bit result.
  const u64 lo_lo = (lhs & 0xFFFFFFFF) * (rhs & 0xFFFFFFFF);
  const u64 hi_lo = (lhs >> 32) * (rhs & 0xFFFFFFFF);
  const u64 lo_hi = (lhs & 0xFFFFFFFF) * (rhs >> 32);
  const u64 hi_hi = (lhs >> 32) * (rhs >> 32);

  /* Now add the products together. These will never overflow. */
  const u64 cross = (lo_lo >> 32) + (hi_lo & 0xFFFFFFFF) + lo_hi;
  const u64 upper = (hi_lo >> 32) + (cross >> 32) + hi_hi;
  return upper;
}

inline u64 umul128h_intrin(u64 lhs, u64 rhs) {
#if defined(__SIZEOF_INT128__)
  // Clang and GCC: native 128-bit integer type. Perform multiply natively, and extract upper 64 bits.
  return static_cast<u64>((static_cast<unsigned __int128>(lhs) * rhs) >> 64);
#elif defined(_MSC_VER)
  // MSVC x64: __umulh gives the high 64 bits of a 64x64 multiply.
  return __umulh(lhs, rhs);
#else
  return umulh128h_cpp(lhs, rhs);
#endif
}

// Compute the high-order part of a 64x64->128 bit multiply, trying to use intrinsics where possible.
constexpr u64 umul128h(u64 lhs, u64 rhs) {
  // If we are being evaluated at compile time, prefer the constexpr implementation, since intrinsics may not be
  // constexpr. Yes, I'm looking at you MSVC __umulh.
  if consteval {
    return umulh128h_cpp(lhs, rhs);
  } else {
    return umul128h_intrin(lhs, rhs);
  }
}
} // namespace bits