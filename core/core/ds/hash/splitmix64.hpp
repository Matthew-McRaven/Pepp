// Modifications by Matthew McRaven to comment, wrap in a namsepace, and remove global state.
/*  Written in 2015 by Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide. This software is distributed without any warranty.

See <http://creativecommons.org/publicdomain/zero/1.0/>. */
#pragma once
#include "core/integers.h"

namespace pepp {

/* This is a fixed-increment version of Java 8's SplittableRandom generator
   See http://dx.doi.org/10.1145/2714064.2660195 and
   http://docs.oracle.com/javase/8/docs/api/java/util/SplittableRandom.html
*/
constexpr u64 splitmix64(u64 z) {
  static const u64 c0 = 0x9e3779b97f4a7c15;
  static const u64 c1 = 0xbf58476d1ce4e5b9;
  static const u64 c2 = 0x94d049bb133111eb;
  z += c0;
  z = (z ^ (z >> 30)) * c1;
  z = (z ^ (z >> 27)) * c2;
  return z ^ (z >> 31);
}
} // namespace pepp