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
// A ClockSink that does nothing but let its clock be scheduled; scheduler.cpp's TestTicker is the fuller version.
struct TestTicker final : public Device, public ClockSink {
  static const inline std::string compatible = "test,jitter_ticker";
  struct Configuration : public Device::Configuration {
    std::string clock;
  };
  explicit TestTicker(Configuration config) : Device(), ClockSink(), _config(config) {}

  void initialize(System *sys) override {
    auto *dev = sys->find_relative(_config.clock, _config.fullname);
    if (!dev) throw std::runtime_error("Ticker: could not find clock " + _config.clock);
    set_clock_source(dev->capability<ClockSource>());
  }
  void reset() override {}
  const Device::Configuration &config() const override { return _config; }
  const Device::ID id() const override { return _config.id; }
  Device::Type type() const override { return Device::Type::ClockSink; }
  std::unique_ptr<DeviceSerializer> serializer() const override { return nullptr; }

  void clock_tick(PulseIndex, u64) override {}
  void set_clock_source(const ClockSource *src) override { _clk = src; }
  const ClockSource *clock_source() const override { return _clk; }

private:
  Configuration _config;
  const ClockSource *_clk = nullptr;
};

pepp::JitteryClock::Configuration clock_cfg(std::string basename, u64 period, u64 jitter, u64 seed) {
  return pepp::JitteryClock::Configuration{
      Device::Configuration{.basename = std::move(basename), .compatible = pepp::JitteryClock::compatible}, period,
      jitter, seed};
}

TestTicker::Configuration ticker_cfg(std::string basename, std::string clock) {
  return TestTicker::Configuration{
      Device::Configuration{.basename = std::move(basename), .compatible = TestTicker::compatible}, std::move(clock)};
}
} // namespace

TEST_CASE("JitteryClock", "[scope:core][scope:core.sim][kind:unit][arch:*]") {
  SECTION("Different seeds keep two same-period clocks from ticking in lockstep") {
    auto sys = std::make_unique<System>(System::Configuration{{.basename = "/"}});
    sys->make_device<pepp::JitteryClock>(clock_cfg("a", 100, 49, 1));
    sys->make_device<pepp::JitteryClock>(clock_cfg("b", 100, 49, 2));
    sys->make_device<TestTicker>(ticker_cfg("ticker_a", "/a"));
    sys->make_device<TestTicker>(ticker_cfg("ticker_b", "/b"));
    sys->initialize();

    std::vector<Device::ID> order;
    for (int i = 0; i < 30; i++) order.push_back(std::get<0>(sys->tick()));

    // With matching periods without jitter the pattern would be abab... forever.
    bool strictly_alternates = true;
    for (std::size_t i = 2; i < order.size(); i++) {
      if (order[i] != order[i % 2]) {
        strictly_alternates = false;
        break;
      }
    }
    CHECK_FALSE(strictly_alternates);
  }
}
