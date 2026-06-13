#pragma once
#include <map>
#include <memory>
#include <span>
#include <vector>
#include "./sim_device.hpp"
#include "core/ds/hash/splitmix64.hpp"
#include "core/integers.h"
#include "core/math/bitmanip/umulh.hpp"
#include "event_loop.hpp"
#include "flat/flat_map.hpp"
#include "sim_eventhandle.hpp"

namespace pepp {

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

  constexpr PulseIndex index_of(u64 tick) const { return PulseIndex{(tick + period / 2) / period}; }
  constexpr u64 edge_time(PulseIndex n) const noexcept { return n.value * period + uniform_jitter(n); }
  // Produce a uniformly random value in [-jitter, -jitter] with no internal state updates.
  constexpr i64 uniform_jitter(PulseIndex n) const noexcept {
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
  constexpr u64 next_clock_tick(u64 tick, u8 delay_cycles = 1) const noexcept;
  bool operator==(const PulseSchedule &rhs) const noexcept = default;
};

struct AClock : public Device {
  AClock(Device::Descriptor desc) : Device(std::move(desc)) {}
  virtual constexpr PulseSchedule schedule() const = 0;
  constexpr Device::ID id() const { return Device::descriptor().id; }
};

// Describe a jitter-free clock that operates at a fixed frequency
struct IdealClock : AClock {
  IdealClock(Device::Descriptor desc, u64 period) : AClock(std::move(desc)), _sched({.period = period}) {}

  constexpr PulseSchedule schedule() const override { return _sched; }

private:
  PulseSchedule _sched;
};
consteval void allow_opaque_handle_increment(PulseSchedule::PulseIndex);
consteval void allow_opaque_handle_add(PulseSchedule::PulseIndex);

struct ScaledClock : public AClock {
  // if jitter_scale is nont-a-number, it will copy the value from period_scale
  ScaledClock(Device::Descriptor desc, std::shared_ptr<AClock> parent, float period_scale,
              float jitter_scale = std::numeric_limits<double>::quiet_NaN())
      : AClock(std::move(desc)), _parent(std::move(parent)), _period_scale(period_scale) {
    _jitter_scale = (jitter_scale != jitter_scale) ? period_scale : jitter_scale;
  }
  constexpr PulseSchedule schedule() const override {
    const auto par = _parent->schedule();
    return PulseSchedule{.period = static_cast<u64>(par.period * _period_scale),
                         .jitter = static_cast<u64>(par.jitter * _jitter_scale),
                         .seed = par.seed};
  }

private:
  float _period_scale, _jitter_scale;
  std::shared_ptr<AClock> _parent;
};

// A clock node which can choose between multiple parent clocks.
struct MuxClock : public AClock {
  // Connect to clock index 0 by default.
  template <typename... Choices>
  explicit MuxClock(Device::Descriptor desc, Choices &&...choices)
      : AClock(std::move(desc)), _index(0), _choices{std::forward<Choices>(choices)...} {
    if (_choices.size() == 0) throw std::runtime_error("MuxClockNode must have at least one choice");
  }

  void select_clock(u16 index) {
    if (index >= _choices.size()) throw std::runtime_error("MuxClockNode: index out of range");
    else if (index == _index) return; // No change
    _index = index;
  }
  std::span<std::shared_ptr<AClock>> choices() { return _choices; }
  constexpr PulseSchedule schedule() const override { return _choices[_index]->schedule(); }

private:
  const AClock *selected_clock() const {
    if (_index > _choices.size()) return nullptr;
    else return _choices[_index].get();
  }
  u16 _index = -1;
  std::vector<std::shared_ptr<AClock>> _choices;
};

// It understands how the internal clock tree maps to Device::IDs that consume it.
// When a device finished consuming a clock pulse, it must notify ClockGovernor that it wants to be recheduled.
// This class must maintain at most 1 clock event per registered device. If that event is paused (because the underlying
// device needed more than 1 cycle to complete), no clock pulses may be scheduled for that device until the event is
// unpaused.
/*
 * The root node of the clock tree. All of the system's clocks are contained within this instance. Devices using a clock
 * must register with this governor.
 *
 * My intention is that "cycle accurate" devices like the microcode CPU will consume 1 clock pulse per cycle, and less
 * accurate devices (like an ISA3 simulation) will consume a clock pulse per instruction. Regardless of the timing
 * accuracy, the device must notify the GlockGovernor when it has completed all work for a clock pulse, which allows the
 * governor to schedule the next clock pulse for the device. That callback is required so the governor can adjust
 * frequency, force instruction interleavings in multi-core systems, or move the device into an idle state.
 */
class ClockGovernor : public EventHandlingDevice {
public:
  ClockGovernor(Device::Descriptor desc, EventLoop &l) : EventHandlingDevice(std::move(desc)), _loop(l) {}
  template <typename ConcreteClock, typename... Args>
  // Make clock transfers ownership of the allocated device!
  ConcreteClock *make_clock(Device::Descriptor desc, Args &&...args);

  void map_device_clock(Device::ID device, Device::ID clock);
  // Simulate "skipping" delay_cycles cycles before scheduling.
  // For timing-accurate cores, should always be 1.
  void request_clock(Device::ID device, u8 delay_cycles = 1);
  // Only handles UpdateClockScheduleEvent
  void handle_event(const Event *ev) override;

private:
  EventLoop &_loop;
  std::vector<Device::ID> _device_to_clock;
  // index is a device ID, and some entries are null.
  std::vector<ClockReceipt *> _device_to_event;
  // A densely-packed vector (without duplicates) of the events which need to be scheduled.
  std::vector<ClockReceipt *> _events_to_schedule;
  std::map<Device::ID, std::pair<AClock *, PulseSchedule>> _clocks;
};

template <typename ConcreteClock, typename... Args>
ConcreteClock *ClockGovernor::make_clock(Descriptor desc, Args &&...args) {
  static_assert(std::derived_from<ConcreteClock, AClock>, "Clock must derive from AClock");
  const auto ret = new ConcreteClock(std::move(desc), std::forward<Args>(args)...);
  _clocks[desc.id] = std::make_pair(ret, PulseSchedule{});
  return ret;
}
} // namespace pepp