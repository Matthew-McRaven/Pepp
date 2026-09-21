#pragma once
#include <memory>
#include <span>
#include <vector>
#include "core/integers.h"
#include "core/sim/api/clock.hpp"

namespace pepp {

// Describe a jitter-free clock that operates at a fixed frequency
struct IdealClock final : public Device, public ClockSource {
  static const inline std::string compatible = "clock,ideal";
  struct Configuration : public Device::Configuration {
    u64 period = 0;
  };
  IdealClock(Configuration config) : Device(), ClockSource(), _config(config), _sched({.period = config.period}) {}

  PulseSchedule schedule() const override { return _sched; }
  void reset() override { _sched = {.period = _config.period}; }
  const Device::Configuration &config() const override { return _config; }
  const Device::ID id() const override { return _config.id; }
  Device::Type type() const override { return Device::Type::ClockSource; }
  const Configuration &casted_config() const { return _config; }
  std::unique_ptr<DeviceSerializer> serializer() const override;
  static std::unique_ptr<DeviceSerializer> make_serializer();

private:
  PulseSchedule _sched;
  Configuration _config;
};

struct ScaledClock final : public Device, public ClockSource {
  static const inline std::string compatible = "clock,scaled";
  struct Configuration : public Device::Configuration {
    float period_scale = 1.0f;
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
  Device::Type type() const override { return Device::Type::ClockSource; }
  const Configuration &casted_config() const { return _config; }
  std::unique_ptr<DeviceSerializer> serializer() const override;
  static std::unique_ptr<DeviceSerializer> make_serializer();

private:
  Configuration _config;
  ClockSource *_parent = nullptr;
};

// A clock node which can choose between multiple parent clocks.
struct MuxClock final : public Device, public ClockSource {
  static const inline std::string compatible = "clock,mux";
  struct Configuration : public Device::Configuration {
    u16 selected = 0;
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
  Device::Type type() const override { return Device::Type::ClockSource; }
  const Configuration &casted_config() const { return _config; }
  std::unique_ptr<DeviceSerializer> serializer() const override;
  static std::unique_ptr<DeviceSerializer> make_serializer();

private:
  const ClockSource *selected_clock() const;
  u16 _index = -1;
  Configuration _config;
  std::vector<ClockSource *> _choices;
};

} // namespace pepp