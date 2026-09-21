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

TEST_CASE("Populate schedule from System's devices", "[scope:core][scope:core.sim][kind:unit][arch:*]") {
  SECTION("Clocks with different rates interleave") {
    auto sys = make_system();
    sys->make_device<pepp::IdealClock>(clock_cfg("slow", 30));
    sys->make_device<pepp::IdealClock>(clock_cfg("fast", 10));
    auto *slow = sys->make_device<TestTicker>(ticker_cfg("slow_ticker", "/slow"));
    auto *fast = sys->make_device<TestTicker>(ticker_cfg("fast_ticker", "/fast"));
    sys->initialize();

    std::vector<u64> ticks;
    sys->tick_while([&](Device::ID, u64 tick) {
      ticks.push_back(tick);
      return ticks.size() < 4;
    });
    CHECK(ticks == std::vector<u64>{10, 20, 30, 30});
    CHECK(fast->edges.size() == 3);
    CHECK(slow->edges.size() == 1);
  }
  SECTION("reset() re-initializes clocks to 0") {
    auto sys = make_system();
    sys->make_device<pepp::IdealClock>(clock_cfg("clk", 10));
    auto *ticker = sys->make_device<TestTicker>(ticker_cfg("ticker", "/clk"));
    sys->initialize();

    for (int i = 0; i < 3; ++i) sys->tick();
    sys->reset();
    CHECK(ticker->edges.empty());
    auto [id, tick] = sys->tick();
    CHECK(id == ticker->id());
    CHECK(tick == 10);
  }
  SECTION("Schedule cannot tick empty system") {
    auto sys = make_system();
    sys->make_device<pepp::IdealClock>(clock_cfg("clk", 10));
    sys->initialize();
    auto [id, tick] = sys->tick();
    CHECK(id == Device::ID{});
    CHECK(tick == -1);
  }
}

TEST_CASE("Scheduler rejects unusable clocks", "[scope:core][scope:core.sim][kind:unit][arch:*][!throws]") {
  SECTION("a sink with no clock source") {
    auto sys = make_system();
    sys->make_device<TestTicker>(ticker_cfg("ticker", ""));
    CHECK_THROWS_AS(sys->initialize(), std::logic_error);
  }
  SECTION("a clock with a zero period") {
    auto sys = make_system();
    sys->make_device<pepp::IdealClock>(clock_cfg("clk", 0));
    sys->make_device<TestTicker>(ticker_cfg("ticker", "/clk"));
    CHECK_THROWS_AS(sys->initialize(), std::logic_error);
  }
}
