#pragma once
#include <memory>
#include <span>
#include <vector>
#include "core/integers.h"
#include "core/sim/api/clock.hpp"

namespace pepp {

// Describe a jitter-free clock that operates at a fixed frequency
struct IdealClock final : public Device, public ClockSource {
  struct Configuration : public Device::Configuration {
    u64 period;
  };
  IdealClock(Configuration config) : Device(), ClockSource(), _config(config), _sched({.period = config.period}) {}

  PulseSchedule schedule() const override { return _sched; }
  void reset() override { _sched = {.period = _config.period}; }
  const Device::Configuration &config() const override { return _config; }
  const Device::ID id() const override { return _config.id; }
  std::unique_ptr<DeviceSerializer> serializer() const override;

private:
  PulseSchedule _sched;
  Configuration _config;
};

struct ScaledClock final : public Device, public ClockSource {
  struct Configuration : public Device::Configuration {
    float period_scale;
    // If not-a-number, configured devices will copy the value from period_scale
    float jitter_scale = std::numeric_limits<float>::quiet_NaN();
    std::string parent_name;
  };

  ScaledClock(Configuration config);
  void initialize(System *) override;
  // schedule is compute on demand and this class otherwise has no state
  void reset() override {}
  PulseSchedule schedule() const override;
  const Device::Configuration &config() const override { return _config; }
  const Device::ID id() const override { return _config.id; }
  std::unique_ptr<DeviceSerializer> serializer() const override;

private:
  Configuration _config;
  ClockSource *_parent = nullptr;
};

// A clock node which can choose between multiple parent clocks.
struct MuxClock final : public Device, public ClockSource {
  struct Configuration : public Device::Configuration {
    u16 selected;
    std::vector<std::string> names;
  };

  // Connect to clock index 0 by default.
  template <typename... Choices>
  explicit MuxClock(Configuration config, Choices &&...choices)
      : Device(), ClockSource(), _index(0), _config(config), _choices{std::forward<Choices>(choices)...} {
    if (_choices.size() == 0) throw std::runtime_error("MuxClockNode must have at least one choice");
  }

  explicit MuxClock(Configuration config);
  void initialize(System *) override;

  void select_clock(u16 index);
  // TODO: selected is ignored at construction time, so it is also ignored here.
  void reset() override { _index = 0; }
  std::span<ClockSource *> choices() { return _choices; }
  PulseSchedule schedule() const override;
  const Device::Configuration &config() const override { return _config; }
  const Device::ID id() const override { return _config.id; }
  std::unique_ptr<DeviceSerializer> serializer() const override;

private:
  const ClockSource *selected_clock() const;
  u16 _index = -1;
  Configuration _config;
  std::vector<ClockSource *> _choices;
};

} // namespace pepp