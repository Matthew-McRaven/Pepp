#pragma once
#include <memory>
#include <span>
#include <vector>
#include "core/integers.h"
#include "core/sim/api/clock.hpp"

namespace pepp {

// Describe a jitter-free clock that operates at a fixed frequency
struct IdealClock : public Device, public ClockSource {
  IdealClock(Device::Descriptor desc, Device::ID id, u64 period)
      : Device(std::move(desc), id), ClockSource(), _sched({.period = period}) {}

  constexpr PulseSchedule schedule() const override { return _sched; }

private:
  PulseSchedule _sched;
};

struct ScaledClock : public Device, public ClockSource {
  // if jitter_scale is nont-a-number, it will copy the value from period_scale
  ScaledClock(Device::Descriptor desc, Device::ID id, std::shared_ptr<ClockSource> parent, float period_scale,
              float jitter_scale = std::numeric_limits<double>::quiet_NaN())
      : Device(std::move(desc), id), ClockSource(), _parent(std::move(parent)), _period_scale(period_scale) {
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
  std::shared_ptr<ClockSource> _parent;
};

// A clock node which can choose between multiple parent clocks.
struct MuxClock : public Device, public ClockSource {
  // Connect to clock index 0 by default.
  template <typename... Choices>
  explicit MuxClock(Device::Descriptor desc, Device::ID id, Choices &&...choices)
      : Device(std::move(desc), id), ClockSource(), _index(0), _choices{std::forward<Choices>(choices)...} {
    if (_choices.size() == 0) throw std::runtime_error("MuxClockNode must have at least one choice");
  }

  void select_clock(u16 index) {
    if (index >= _choices.size()) throw std::runtime_error("MuxClockNode: index out of range");
    else if (index == _index) return; // No change
    _index = index;
  }
  std::span<std::shared_ptr<ClockSource>> choices() { return _choices; }
  constexpr PulseSchedule schedule() const override { return _choices[_index]->schedule(); }

private:
  const ClockSource *selected_clock() const {
    if (_index > _choices.size()) return nullptr;
    else return _choices[_index].get();
  }
  u16 _index = -1;
  std::vector<std::shared_ptr<ClockSource>> _choices;
};

} // namespace pepp