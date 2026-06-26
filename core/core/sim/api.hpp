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

#include <functional>
#include <string>
#include "core/ds/hash/splitmix64.hpp"
#include "core/ds/opaque_handle.hpp"
#include "core/integers.h"
#include "core/math/bitmanip/enums.hpp"
#include "core/math/bitmanip/span.hpp"
#include "core/math/bitmanip/swap.hpp"
#include "core/math/bitmanip/umulh.hpp"
#include "core/math/geom/interval.hpp"

using Tick = u32;
using Address = u32;
using AddressSpan = pepp::core::Interval<Address>;

struct Device {
  using ID = pepp::OpaqueHandle<struct DeviceID, u8>;
  using IDGenerator = std::function<Device::ID()>;
  struct Descriptor {
    ID id = Device::ID{0};
    std::string basename = "", fullname = "", compatible = "";
    std::string child_name(std::string_view child_basename) const;
  };
  // Bitflags telling you what interfaces this abstract device implements.
  // e.g., if any(type() & Type::MemoryTarget), then this device implements the MemoryTarget interface.
  // You could then get a pointer to the interface by calling capability(Type::MemoryTarget).
  // It is a bitmask, allowing a device to implement multiple interfaces.
  enum class Type : u64 {
    None = 0,
    Root = 1,
    // Generic device types
    MemoryTarget = Root << 1,
    MemoryTranslator = MemoryTarget << 1,
    MemoryInitiator = MemoryTranslator << 1,
    ClockSource = MemoryInitiator << 1,
    ClockSink = ClockSource << 1,
    MASK = (ClockSink << 1) - 1,
  };

  Device(Descriptor desc) : _desc(desc) {}
  virtual ~Device() = default;
  const Descriptor &descriptor() const { return _desc; }
  // Helper to test if this device implements a particular interface type.
  virtual Type type() const { return Type::None; }
  // Features specific to the concrete  instance of the device.
  virtual u64 features() const { return 0; }
  // Given one of the interface types, return an instance of that interface if this device implements it, otherwise
  // return nullptr.
  template <typename Concrete> Concrete *capability() {
    using namespace bits;
    if (!any(type() & Concrete::TypeMask)) return nullptr;
    Device *p = capability(Concrete::TypeMask);
    return dynamic_cast<Concrete *>(p);
  }

protected:
  // Subclasses should override this to return a pointer to the appropriate interface if the requested type is
  // supported, otherwise nullptr.
  virtual Device *capability(Device::Type t);

private:
  Descriptor _desc;
};
consteval void is_bitflags(Device::Type);

// If select memory operations fail (e.g., lack of MMI, unmapped address in
// bus), specify the behavior of the target.
enum class FailPolicy {
  YieldDefaultValue, // The target picks some arbitrary default value, and
  // returns it successfully.
  RaiseError // The target returns an appropriate error message.
};

// In API v1, there was a concept of effectful and speculative.
// effectful=false implied that the memory operation was triggered by the UI.
// speculative=true implied some form of memory pre-fetch triggered by the
// simulated hardware. In no case was effectful=false, speculative=true a
// meaningful combination.
//
// In API v2, these have been condensed into a Operation::Type, eliminating the
// impossible case.
struct Operation {
  enum class Type : u8 {
    // Access triggered by the application / user interface.
    // Should not trigger memory-mapped IO, cache misses, etc.
    Application = 1,
    // Speculative access triggered within the simulation. Probably shouldn't
    // trigger
    // MMIO, but this is hardware dependent.
    Speculative = 2,
    // Access triggered by the simulator while performing some analysis
    // operation.
    // It must never trigger memory-mapped IO nor is it allowed to emit trace
    // events.
    BufferInternal = 3,
    // Non-speculative access triggered within the simulation. Should trigger
    // memory mapped IO,
    // cache updates, etc.
    Standard = 0,
  } type = Type::Standard;
  enum class Kind : bool { instruction = false, data = true } kind;
};

struct Target {
  struct Result {
    // Number of simulation ticks required to complete the memory op.
    // 0 indicates the operation completed immediately.
    Tick delay;
    // Did the memory access trigger a breakpoint?
    bool pause;
  };

  static constexpr Device::Type TypeMask = Device::Type::MemoryTarget;
  virtual ~Target() = default;
  // Needed by Translators to perform standalone address translation.
  virtual Device::ID device_ID() const = 0;
  virtual Device::Descriptor device() const = 0;

  virtual AddressSpan span() const = 0;
  virtual Result read(Address address, bits::span<u8> dest, Operation op) const = 0;
  virtual Result write(Address address, bits::span<const u8> src, Operation op) = 0;
  virtual void clear(u8 fill) = 0;

  // If dest is larger than maxOffset-minOffset+1, copy bytes from this target
  // to the span.
  virtual void dump(bits::span<u8> dest) const = 0;

  // These convenience methods are so commonly used that they tend to be declared inline in multiple project locations.
  template <std::integral I, bool byteswap = false> std::pair<Result, I> read(Address address, Operation op) const;
  template <std::integral I, bool byteswap = false> Result write(Address address, I src, Operation op);
};

// If you act like a bus you need to implement this. It allows decoding of packets into their initiator's address space.
struct Translator {
  static constexpr Device::Type TypeMask = Device::Type::MemoryTranslator;
  virtual ~Translator() = default;
  virtual std::tuple<bool, Device::ID, Address> forward(Address address) const = 0;
  virtual std::optional<Address> backward(Device::ID child, Address address) const = 0;
  // virtual void setPathManager(QSharedPointer<api2::Paths> paths) = 0;
  // virtual QSharedPointer<const api2::Paths> pathManager() const = 0;
};

struct Initiator {
  static constexpr Device::Type TypeMask = Device::Type::MemoryInitiator;
  virtual ~Initiator() = default;
  // Sets the memory backing for a particular port (i.e., set the I and D caches
  // separately) If port is nullptr, then all ports will use the target,
  virtual void bind_port(Target *target, std::string_view port_name = {}) = 0;
  virtual const std::span<const std::string> list_ports() const = 0;
  virtual Target *get_port(std::string_view port_name = {}) const = 0;
};

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

/*
 * Inline implementations
 */
template <std::integral I, bool byteswap>
std::pair<Target::Result, I> Target::read(Address address, Operation op) const {
  I dest;
  auto r = read(address, bits::span<u8>(reinterpret_cast<u8 *>(&dest), sizeof(I)), op);
  if constexpr (byteswap) dest = bits::byteswap(dest);
  return {r, dest};
}
template <std::integral I, bool byteswap> Target::Result Target::write(Address address, I src, Operation op) {
  if constexpr (byteswap) src = bits::byteswap(src);
  return write(address, bits::span<const u8>(reinterpret_cast<const u8 *>(&src), sizeof(I)), op);
}

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
