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
#include <array>
#include <catch.hpp>
#include "core/sim/clocktree.hpp"
#include "core/sim/debugger/trace_device.hpp"
#include "core/sim/debugger/tvm_interpreter.hpp"
#include "core/sim/debugger/tvm_tracebuffer.hpp"
#include "core/sim/memory/io/state.hpp"
#include "core/sim/system.hpp"
#include "core/sim/systemparser.hpp"

namespace {
constexpr Device::ID CPU{1}; // Stands in for the CPU whose instruction caused the changes.
const Operation cpu_op(Operation::Type::Standard, Operation::Kind::data, CPU);

// Counts settle() calls, to show that each interpreter run settles the system exactly once.
struct SettleCounter final : public Device {
  struct Configuration : public Device::Configuration {};
  explicit SettleCounter(Configuration config) : _config(config) {}
  void reset() override {}
  void settle() override { count++; }
  const Device::Configuration &config() const override { return _config; }
  const Device::ID id() const override { return _config.id; }
  std::unique_ptr<DeviceSerializer> serializer() const override { return nullptr; }
  int count = 0;

private:
  Configuration _config;
};

pepp::IdealClock::Configuration clock_cfg(std::string name, u64 period, std::optional<pepp::ClockEnable::Mode> mode) {
  pepp::IdealClock::Configuration cfg{{.basename = std::move(name), .compatible = pepp::IdealClock::compatible},
                                      period};
  if (mode) cfg.enable = pepp::ClockEnable::Configuration{.source = "/ctl", .mode = *mode};
  return cfg;
}
} // namespace

TEST_CASE("trace::Recorder: clock enables and selections", "[scope:core][scope:core.dbg][kind:unit][arch:*]") {
  using E = pepp::ClockEnable;
  // /latched runs until /ctl is written, /gated runs while /ctl is non-zero, and /mux selects between the two.
  System sys;
  auto *ctl = sys.make_device<StateRegister>(StateRegister::Configuration{
      {.basename = "ctl", .compatible = StateRegister::compatible}, 0, AddressSpan(0, 0)});
  auto *latched = sys.make_device<pepp::IdealClock>(clock_cfg("latched", 10, E::DisableOnWrite{}));
  auto *gated = sys.make_device<pepp::IdealClock>(clock_cfg("gated", 20, E::EnableWhenAny{}));
  auto *mux = sys.make_device<pepp::MuxClock>(pepp::MuxClock::Configuration{
      {.basename = "mux", .compatible = pepp::MuxClock::compatible}, 0, {"/latched", "/gated"}});
  auto *settles = sys.make_device<SettleCounter>(SettleCounter::Configuration{{.basename = "settles"}});
  auto *tbdev = sys.make_device<trace::BufferDevice>(
      trace::BufferDevice::Configuration{Device::Configuration{.basename = "trace"}, 4});
  sys.initialize();
  for (const Device *dev : std::initializer_list<const Device *>{ctl, latched, gated, mux})
    tbdev->trace(dev->id(), true);

  auto instruction = [&](auto &&body) {
    tbdev->buffer().begin(CPU);
    body();
    return tbdev->buffer().commit(CPU);
  };
  auto replay = [&](tvm::ProgramLocation loc, tvm::Direction dir) {
    auto blaster = sys.make_trace_interpreter();
    blaster->backend().set_direction(dir);
    blaster->run(loc);
    REQUIRE(blaster->stop_cause() == tvm::StopCause::None);
  };

  SECTION("Registers are exposed") {
    auto *scan = sys.register_scan();
    CHECK(scan->find("/latched:enabled").has_value());
    CHECK(scan->find("/gated:enabled").has_value());
    CHECK(scan->find("/mux:selected").has_value());
  }
  SECTION("Undoing a write restores every enable mode") {
    const auto loc = instruction([&] { ctl->write<u8>(0, 1, cpu_op); });
    CHECK(!latched->self_enabled());
    CHECK(gated->self_enabled());

    replay(loc, tvm::Direction::Backward);
    CHECK(latched->self_enabled());
    CHECK(!gated->self_enabled());

    replay(loc, tvm::Direction::Forward);
    CHECK(!latched->self_enabled());
    CHECK(gated->self_enabled());
  }
  SECTION("Each run settles the system once") {
    const std::array<tvm::ProgramLocation, 2> locs{instruction([&] { ctl->write<u8>(0, 1, cpu_op); }),
                                                   instruction([&] { mux->select_clock(1); })};
    const auto before = settles->count;
    replay(locs[0], tvm::Direction::Forward);
    CHECK(settles->count == before + 1);
    sys.make_trace_interpreter()->run_each(locs);
    CHECK(settles->count == before + 2);
  }
  SECTION("Undoing a selection restores the previous clock") {
    const auto loc = instruction([&] { mux->select_clock(1); });
    CHECK(mux->schedule().period == 20);
    replay(loc, tvm::Direction::Backward);
    CHECK(mux->schedule().period == 10);
    replay(loc, tvm::Direction::Forward);
    CHECK(mux->schedule().period == 20);
  }
}
