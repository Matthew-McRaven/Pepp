
/*
 * Copyright (c) 2024-2026 J. Stanley Warford, Matthew McRaven
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

#include "core/ds/hash/splitmix64.hpp"
#include "core/ds/opaque_handle.hpp"
#include "core/integers.h"
#include "core/math/bitmanip/umulh.hpp"
#include "core/sim/api/device.hpp"

/*
 * Essential a POD class which encodes the information from the clock-tree in a way that is fast and deterministic to
 * schedule.
 *
 * The way we implement jitter is a bit strange, and it dervies from my time-reversible simulation requirements.
 * I need the jitter to be a function of the current time and the current time only, otherwise I need to serialize clock
 * internals to the trace buffer. This would probably exceed the size of the instruction trace and would be bad.
 *
 * Given a clock period, we can split up all of time into period-sized intervals centered
 * around a multiple of the period minus one-half. e.g, if period is 100 [0, 49] is interval #0, [50, 149] is interval
 * #1, [150, 249] is interval #2. That interval index is called PulseIndex.
 *
 * Jitter is computed deterministically from the PulseIndex and an unchaning seed. Because of my use of intervals, any
 * tick within an interval can be used to recover the PulseIndex, which can be used to extract when the (jittery) clock
 * is expected for that PulseIndex.
 *
 * This means the actual scheduling computation has no state (unlike an approach based on accumulating jitter) and we
 * can fast-forward indefinitely. That being said, jitter is not uniformly distributed, and over time clocks will drift
 * with respect to each other, which cannot be represented by this deterministic schedule.
 */
struct PulseSchedule {

  using PulseIndex = pepp::OpaqueHandle<struct ClockPulseTag, u64>;

  // Period in ns
  u64 period = 0;
  // must be < 1/2 period
  u64 jitter = 0;
  // Bits that are XOR'ed in when computing jitter from index. Useful to prevent two clocks with the same
  u64 seed = 0xfeeddeadbeefcafe;

  constexpr PulseIndex index_of(u64 tick) const;
  constexpr u64 edge_time(PulseIndex n) const noexcept;
  // Produce a uniformly random value in [-jitter, -jitter] with no internal state updates.
  constexpr i64 uniform_jitter(PulseIndex n) const noexcept;
  constexpr u64 next_clock_tick(u64 tick, u8 delay_cycles = 1) const noexcept;
  bool operator==(const PulseSchedule &rhs) const noexcept = default;
};
consteval void allow_opaque_handle_increment(PulseSchedule::PulseIndex);
consteval void allow_opaque_handle_add(PulseSchedule::PulseIndex);

struct ClockSource {
  static constexpr Device::Type TypeMask = Device::Type::ClockSource;
  virtual ~ClockSource() = default;
  virtual PulseSchedule schedule() const = 0;
};

struct ClockSink {
  static constexpr Device::Type TypeMask = Device::Type::ClockSink;
  virtual ~ClockSink() = default;
  virtual void clock_tick(PulseSchedule::PulseIndex idx, u64 tick) = 0;
  virtual void set_clock_source(const ClockSource *src) = 0;
  virtual const ClockSource *clock_source() const = 0;
};

inline constexpr PulseSchedule::PulseIndex PulseSchedule::index_of(u64 tick) const {
  return PulseIndex{(tick + period / 2) / period};
}

inline constexpr u64 PulseSchedule::edge_time(PulseIndex n) const noexcept {
  return n.value * period + uniform_jitter(n);
}

inline constexpr i64 PulseSchedule::uniform_jitter(PulseIndex n) const noexcept {
  // Max allowable is [-_jitter, +_jitter], which has 2*_jitter non-zero elements plus the element zero.
  const u64 jitter_span = 2 * jitter + 1;
  // Convert passed index to a random number in [0, 2^64-1]
  const u64 rnd = pepp::splitmix64(n.value ^ seed);
  // Use Lemire's trick to convert [0,2^64-1] to [0,jitter_span]. See details at:
  // - https://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction/
  // - https://github.com/lemire/fastrange
  const u64 j = bits::umul128h(rnd, jitter_span);
  // Shift [0, 2*_jitter+1] to [-jitter, +jitter]
  return j - (i64)jitter;
}

inline constexpr u64 PulseSchedule::next_clock_tick(u64 tick, u8 delay_cycles) const noexcept {
  // get pulse index for current tick, increment it
  const auto idx = index_of(tick) + delay_cycles;
  return edge_time(idx);
}
