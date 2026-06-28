#pragma once
#include <memory>
#include <span>
#include <vector>
#include "core/integers.h"
#include "core/sim/api/clock.hpp"

namespace pepp {

// Describe a jitter-free clock that operates at a fixed frequency
struct IdealClock : public Device, ClockSource {
  struct Configuration : public Device::Configuration {
    u64 period;
  };
  IdealClock(Configuration config, Device::ID id)
      : Device(), ClockSource(), _config(config), _id(id), _sched({.period = config.period}) {}

  constexpr PulseSchedule schedule() const override { return _sched; }
  const Configuration &config() const override { return _config; }
  const Device::ID id() const override { return _id; }

private:
  PulseSchedule _sched;
  Configuration _config;
  Device::ID _id;
};

struct ScaledClock : public Device, public ClockSource {
  struct Configuration : public Device::Configuration {
    float period_scale;
    // If not-a-number, configured devices will copy the value from period_scale
    float jitter_scale = std::numeric_limits<double>::quiet_NaN();
  };

  ScaledClock(Configuration config, Device::ID id, std::shared_ptr<ClockSource> parent)
      : Device(), ClockSource(), _config(config), _id(id), _parent(std::move(parent)) {
    if (_config.jitter_scale != _config.jitter_scale) _config.jitter_scale = _config.period_scale;
  }
  constexpr PulseSchedule schedule() const override {
    const auto par = _parent->schedule();
    return PulseSchedule{.period = static_cast<u64>(par.period * _config.period_scale),
                         .jitter = static_cast<u64>(par.jitter * _config.jitter_scale),
                         .seed = par.seed};
  }
  const Configuration &config() const override { return _config; }
  const Device::ID id() const override { return _id; }

private:
  Configuration _config;
  Device::ID _id;
  std::shared_ptr<ClockSource> _parent;
};

// A clock node which can choose between multiple parent clocks.
struct MuxClock : public Device, public ClockSource {
  struct Configuration : public Device::Configuration {
    // No additional configuration for now.
  };
  // Connect to clock index 0 by default.
  template <typename... Choices>
  explicit MuxClock(Configuration config, Device::ID id, Choices &&...choices)
      : Device(), ClockSource(), _index(0), _config(config), _id(id), _choices{std::forward<Choices>(choices)...} {
    if (_choices.size() == 0) throw std::runtime_error("MuxClockNode must have at least one choice");
  }

  void select_clock(u16 index) {
    if (index >= _choices.size()) throw std::runtime_error("MuxClockNode: index out of range");
    else if (index == _index) return; // No change
    _index = index;
  }
  std::span<std::shared_ptr<ClockSource>> choices() { return _choices; }
  constexpr PulseSchedule schedule() const override { return _choices[_index]->schedule(); }
  const Configuration &config() const override { return _config; }
  const Device::ID id() const override { return _id; }

private:
  const ClockSource *selected_clock() const {
    if (_index > _choices.size()) return nullptr;
    else return _choices[_index].get();
  }
  u16 _index = -1;
  Configuration _config;
  Device::ID _id;
  std::vector<std::shared_ptr<ClockSource>> _choices;
};

} // namespace pepp