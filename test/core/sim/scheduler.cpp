/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
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
#include <catch.hpp>
#include <vector>
#include "core/sim/clocktree.hpp"
#include "core/sim/memory/io/state.hpp"
#include "core/sim/system.hpp"
#include "core/sim/systemparser.hpp"

namespace {

// A ClockSink that records every edge it is handed, so a test can read the schedule back out of it.
// Its clock is named rather than pushed in, matching how the CPUs resolve theirs.
struct TestTicker final : public Device, public ClockSink {
  static const inline std::string compatible = "test,ticker";
  struct Configuration : public Device::Configuration {
    // Empty leaves this sink unclocked, which is what populate_scheduler is expected to reject.
    std::string clock;
  };
  explicit TestTicker(Configuration config) : Device(), ClockSink(), _config(config) {}

  void initialize(System *sys) override {
    if (_config.clock.empty()) return;
    auto *dev = sys->find_relative(_config.clock, _config.fullname);
    if (!dev) throw std::runtime_error("Ticker: could not find clock " + _config.clock);
    set_clock_source(dev->capability<ClockSource>());
  }
  void reset() override { edges.clear(); }
  const Device::Configuration &config() const override { return _config; }
  const Device::ID id() const override { return _config.id; }
  Device::Type type() const override { return Device::Type::ClockSink; }
  std::unique_ptr<DeviceSerializer> serializer() const override { return nullptr; }

  void clock_tick(PulseIndex idx, u64 tick) override { edges.push_back({idx, tick}); }
  void set_clock_source(const ClockSource *src) override { _clk = src; }
  const ClockSource *clock_source() const override { return _clk; }

  std::vector<std::tuple<PulseIndex, u64>> edges;

private:
  Configuration _config;
  const ClockSource *_clk = nullptr;
};

pepp::IdealClock::Configuration clock_cfg(std::string basename, u64 period) {
  return pepp::IdealClock::Configuration{
      Device::Configuration{.basename = std::move(basename), .compatible = pepp::IdealClock::compatible}, period};
}

TestTicker::Configuration ticker_cfg(std::string basename, std::string clock) {
  return TestTicker::Configuration{
      Device::Configuration{.basename = std::move(basename), .compatible = TestTicker::compatible}, std::move(clock)};
}

auto make_system() { return std::make_unique<System>(System::Configuration{{.basename = "/"}}); }

} // namespace

TEST_CASE("System scheduler", "[scope:core][scope:core.sim][kind:unit][arch:*][!throws]") {
  auto sys = make_system();
  using V = std::vector<std::tuple<Device::ID, u64>>;
  auto next = [&](int count) {
    V ret;
    for (int i = 0; i < count; i++) ret.push_back(sys->tick());
    return ret;
  };

  SECTION("Clocks with different rates interleave, and reset() restarts them") {
    sys->make_device<pepp::IdealClock>(clock_cfg("slow", 30));
    sys->make_device<pepp::IdealClock>(clock_cfg("fast", 10));
    auto *slow = sys->make_device<TestTicker>(ticker_cfg("slow_ticker", "/slow"));
    auto *fast = sys->make_device<TestTicker>(ticker_cfg("fast_ticker", "/fast"));
    sys->initialize();

    CHECK(next(4) == V{{fast->id(), 10}, {fast->id(), 20}, {slow->id(), 30}, {fast->id(), 30}});
    sys->reset();
    CHECK(fast->edges.empty());
    CHECK(next(1) == V{{fast->id(), 10}});
  }
  SECTION("Sink-less system does not tick") {
    sys->make_device<pepp::IdealClock>(clock_cfg("clk", 10));
    sys->initialize();
    CHECK(sys->tick() == std::tuple<Device::ID, u64>{Device::ID{}, ~u64{0}});
  }
  SECTION("All sinks must have a clock") {
    sys->make_device<TestTicker>(ticker_cfg("ticker", ""));
    CHECK_THROWS_AS(sys->initialize(), std::logic_error);
    auto zero = make_system();
    zero->make_device<pepp::IdealClock>(clock_cfg("clk", 0));
    zero->make_device<TestTicker>(ticker_cfg("ticker", "/clk"));
    CHECK_THROWS_AS(zero->initialize(), std::logic_error);
  }
  SECTION("Schedule responds to enabled clocks") {
    // /gated runs while /ctl is non-zero. /latched runs until /ctl is first written.
    using E = pepp::ClockEnable;
    auto *ctl = sys->make_device<StateRegister>(StateRegister::Configuration{
        {.basename = "ctl", .compatible = StateRegister::compatible}, 0, AddressSpan(0, 0)});
    auto gated_cfg = clock_cfg("gated", 10), latched_cfg = clock_cfg("latched", 35);
    gated_cfg.enable = E::Configuration{.source = "/ctl", .mode = E::EnableWhenAny{}};
    latched_cfg.enable = E::Configuration{.source = "/ctl", .mode = E::DisableOnWrite{}};
    sys->make_device<pepp::IdealClock>(gated_cfg);
    sys->make_device<pepp::IdealClock>(latched_cfg);
    auto *gated = sys->make_device<TestTicker>(ticker_cfg("gated_ticker", "/gated"));
    auto *latched = sys->make_device<TestTicker>(ticker_cfg("latched_ticker", "/latched"));
    auto set_ctl = [&](u8 v) { ctl->write<u8>(0, v, Operation{Operation::Type::Standard, Operation::Kind::data}); };
    sys->initialize();

    // Disabled at initialize.
    CHECK(next(2) == V{{latched->id(), 35}, {latched->id(), 70}});
    // Enabled on the next edge after now, while /latched stops.
    set_ctl(1);
    CHECK(next(3) == V{{gated->id(), 80}, {gated->id(), 90}, {gated->id(), 100}});
    // Nothing left to run.
    set_ctl(0);
    CHECK(std::get<0>(sys->tick()) == Device::ID{});
    sys->reset();
    CHECK(next(1) == V{{latched->id(), 35}});
  }
  SECTION("Update a MuxClock mid-simulation") {
    sys->make_device<pepp::IdealClock>(clock_cfg("a", 10));
    sys->make_device<pepp::IdealClock>(clock_cfg("b", 30));
    auto *mux = sys->make_device<pepp::MuxClock>(pepp::MuxClock::Configuration{
        {.basename = "mux", .compatible = pepp::MuxClock::compatible}, 0, {"/a", "/b"}});
    auto *ticker = sys->make_device<TestTicker>(ticker_cfg("ticker", "/mux"));
    sys->initialize();

    CHECK(next(1) == V{{ticker->id(), 10}});
    mux->select_clock(1);
    CHECK(next(2) == V{{ticker->id(), 30}, {ticker->id(), 60}});
    // A write behind the clock's back, e.g., trace replay, applies once the system settles.
    auto *scan = sys->register_scan();
    scan->write<u16>(*scan->find("/mux:selected"), 0, RegisterScan::Level::Host);
    sys->settle();
    CHECK(next(1) == V{{ticker->id(), 70}});
  }
}
