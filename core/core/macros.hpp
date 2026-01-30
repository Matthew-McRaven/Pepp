#pragma once

#include <utility>
// Prefer C++23 std::unreachable when available.
#if defined(__cpp_lib_unreachable) && (__cpp_lib_unreachable >= 202202L)
#define PEPP_UNREACHABLE() ::std::unreachable()
// MSVC
#elif defined(_MSC_VER)
#define PEPP_UNREACHABLE() __assume(0)

// GCC / Clang
#elif defined(__GNUC__) || defined(__clang__)
#define PEPP_UNREACHABLE() __builtin_unreachable()

#else
#error "PEPP_UNREACHABLE: no supported unreachable intrinsic (need C++23 std::unreachable, MSVC, GCC, or Clang)."
#endif
