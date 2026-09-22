#pragma once
#include <memory>
#include <optional>
#include <span>
#include <variant>
#include <vector>
#include "core/integers.h"
#include "core/math/bitmanip/order.hpp"
#include "core/sim/api/clock.hpp"
#include "core/sim/api/event.hpp"
#include "core/sim/api/memory.hpp"

namespace pepp {

// Allow a clock to be conditionally-enabled
struct ClockEnable {
  // The clock will remain enabled until the watched range is written to and a MemoryWritten event is raised.
  // Then the clock will remain disabled until reset.
  struct DisableOnWrite {};
  // The clock will become enabled if any bit in the watched range is set, and disabled if all bits are clear.
  struct EnableWhenAny {
    u64 mask = ~u64{0};
    bits::Order order = bits::Order::LittleEndian;
  };
  // The clock will become enabled if the watched (range & mask == match), and disabled otherwise.
  struct EnableWhenEqual {
    u64 mask = ~u64{0};
    u64 match = 0;
    bits::Order order = bits::Order::LittleEndian;
  };
  using Mode = std::variant<DisableOnWrite, EnableWhenAny, EnableWhenEqual>;

  struct Configuration {
    // Path to a device which is both a Target and an EventSource.
    std::string source;
    // Address range
    std::optional<AddressSpan> span;
    Mode mode;
  };
};

// A common base class for clocks which provides utilities for handling enable/disable and notifying the System of a
// schedule change.
class ClockNode : public Device, public ClockSource, public EventSource, public EventSink {
public:
  Device::Type type() const override;
  void on_event(Device::ID from, const Event &event) override;
  // Whether this clock's own enable is asserted, ignoring any parent clock.
  bool self_enabled() const;

protected:
  explicit ClockNode(std::optional<ClockEnable::Configuration> enable) : _enable(std::move(enable)) {}
  // Must be called by derived classes!
  void initialize(System *sys) override;
  void reset_enable() { _latched_off = false; }
  PulseSchedule apply_enable(PulseSchedule sched) const;
  void schedule_changed() { raise(id(), UpdateSchedule{}); }

private:
  // Forward-declaration of a visitor to implement self_enabled().
  struct SelfEnabled;
  // Current value of the watched range, as an integer in the given byte order.
  u64 read_watched(bits::Order order) const;
  std::optional<ClockEnable::Configuration> _enable;
  const Target *_source = nullptr;
  AddressSpan _watched{};
  // Only used by DisableOnWrite. The other modes read source each time they are asked, so they hold no state.
  bool _latched_off = false;
};

// Describe a jitter-free clock that operates at a fixed frequency
struct IdealClock final : public ClockNode {
  static const inline std::string compatible = "clock,ideal";
  struct Configuration : public Device::Configuration {
    u64 period = 0;
    std::optional<ClockEnable::Configuration> enable;
  };
  IdealClock(Configuration config) : ClockNode(config.enable), _sched({.period = config.period}), _config(config) {}

  void initialize(System *sys) override { ClockNode::initialize(sys); }
  PulseSchedule schedule() const override { return apply_enable(_sched); }
  void reset() override {
    _sched = {.period = _config.period};
    reset_enable();
  }
  const Device::Configuration &config() const override { return _config; }
  const Device::ID id() const override { return _config.id; }
  const Configuration &casted_config() const { return _config; }
  std::unique_ptr<DeviceSerializer> serializer() const override;
  static std::unique_ptr<DeviceSerializer> make_serializer();

private:
  PulseSchedule _sched;
  Configuration _config;
};

struct ScaledClock final : public ClockNode {
  static const inline std::string compatible = "clock,scaled";
  struct Configuration : public Device::Configuration {
    float period_scale = 1.0f;
    // If not-a-number, configured devices will copy the value from period_scale
    float jitter_scale = std::numeric_limits<float>::quiet_NaN();
    std::string parent_name;
    std::optional<ClockEnable::Configuration> enable;
  };

  ScaledClock(Configuration config);
  void initialize(System *) override;
  // schedule is computed on demand, so only the enable has state
  void reset() override { reset_enable(); }
  PulseSchedule schedule() const override;
  const Device::Configuration &config() const override { return _config; }
  const Device::ID id() const override { return _config.id; }
  const Configuration &casted_config() const { return _config; }
  std::unique_ptr<DeviceSerializer> serializer() const override;
  static std::unique_ptr<DeviceSerializer> make_serializer();

private:
  Configuration _config;
  ClockSource *_parent = nullptr;
};

// A clock node which can choose between multiple parent clocks.
struct MuxClock final : public ClockNode {
  static const inline std::string compatible = "clock,mux";
  struct Configuration : public Device::Configuration {
    u16 selected = 0;
    std::vector<std::string> names;
    std::optional<ClockEnable::Configuration> enable;
  };

  // Connect to clock index 0 by default.
  template <typename... Choices>
  explicit MuxClock(Configuration config, Choices &&...choices)
      : ClockNode(config.enable), _index(0), _config(config), _choices{std::forward<Choices>(choices)...} {
    if (_choices.size() == 0) throw std::runtime_error("MuxClockNode must have at least one choice");
  }

  explicit MuxClock(Configuration config);
  void initialize(System *) override;

  // Raises UpdateSchedule when the selection changes.
  void select_clock(u16 index);
  // TODO: selected is ignored at construction time, so it is also ignored here.
  void reset() override {
    _index = 0;
    reset_enable();
  }
  std::span<ClockSource *> choices() { return _choices; }
  PulseSchedule schedule() const override;
  const Device::Configuration &config() const override { return _config; }
  const Device::ID id() const override { return _config.id; }
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