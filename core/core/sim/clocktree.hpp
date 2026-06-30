#pragma once
#include <memory>
#include <span>
#include <vector>
#include "core/integers.h"
#include "core/sim/api/clock.hpp"

namespace pepp {

// Describe a jitter-free clock that operates at a fixed frequency
struct IdealClock : public Device, ClockSource {
  struct Configuration {
    Device::Configuration base;
    u64 period;
  };
  IdealClock(Configuration config, Device::ID id)
      : Device(), ClockSource(), _config(config), _id(id), _sched({.period = config.period}) {}

  PulseSchedule schedule() const override { return _sched; }
  const Device::Configuration &config() const override { return _config.base; }
  const Device::ID id() const override { return _id; }

private:
  PulseSchedule _sched;
  Configuration _config;
  Device::ID _id;
};

struct ScaledClock : public Device, public ClockSource {
  struct Configuration {
    Device::Configuration base;
    float period_scale;
    // If not-a-number, configured devices will copy the value from period_scale
    float jitter_scale = std::numeric_limits<double>::quiet_NaN();
  };
  struct DeferredConfiguration {
    std::string parent_name;
  };

  ScaledClock(Configuration config, Device::ID id, DeferredConfiguration deferred);
  void initialize(System *) override;
  PulseSchedule schedule() const override;
  const Device::Configuration &config() const override { return _config.base; }
  const Device::ID id() const override { return _id; }

private:
  Device::ID _id;
  Configuration _config;
  DeferredConfiguration _deferred;
  ClockSource *_parent = nullptr;
};

// A clock node which can choose between multiple parent clocks.
struct MuxClock : public Device, public ClockSource {
  struct Configuration {
    Device::Configuration base;
  };
  struct DeferredConfiguration {
    int selected = 0;
    std::vector<std::string> names;
  };

  // Connect to clock index 0 by default.
  template <typename... Choices>
  explicit MuxClock(Configuration config, Device::ID id, Choices &&...choices)
      : Device(), ClockSource(), _index(0), _config(config), _id(id), _choices{std::forward<Choices>(choices)...} {
    if (_choices.size() == 0) throw std::runtime_error("MuxClockNode must have at least one choice");
  }

  explicit MuxClock(Configuration config, Device::ID id, DeferredConfiguration choices);
  void initialize(System *) override;

  void select_clock(u16 index);
  std::span<ClockSource *> choices() { return _choices; }
  PulseSchedule schedule() const override;
  const Device::Configuration &config() const override { return _config.base; }
  const Device::ID id() const override { return _id; }

private:
  const ClockSource *selected_clock() const;
  u16 _index = -1;
  Device::ID _id;
  Configuration _config;
  DeferredConfiguration _deferred;

  std::vector<ClockSource *> _choices;
};

} // namespace pepp