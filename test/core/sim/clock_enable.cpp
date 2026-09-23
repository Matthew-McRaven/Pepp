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
#include "core/sim/clocktree.hpp"
#include "core/sim/memory/io/state.hpp"
#include "core/sim/system.hpp"
#include "core/sim/systemparser.hpp"

namespace {
struct ScheduleCounter final : public EventSink {
  void on_event(Device::ID, const Event &event) override {
    count += dynamic_cast<const UpdateSchedule *>(&event) != nullptr;
  }
  int count = 0;
};

std::unique_ptr<System> make_system(const std::string &clk_enable, const std::string &child_enable, u8 fill) {
  auto sys = parse_system(R"({"children": [
    {"compatible": "io,state", "basename": "ctl", "offset": 0, "width": 2, "fill": )" +
                          std::to_string(fill) + R"(},
    {"compatible": "clock,ideal", "basename": "clk", "period": 10, "enable": )" +
                          clk_enable + R"(},
    {"compatible": "clock,scaled", "basename": "child", "parent": "/clk", "period_scale": 2, "enable": )" +
                          child_enable + R"(},
    {"compatible": "clock,mux", "basename": "mux", "choices": ["/clk", "/child"]}
  ]})");
  sys->initialize();
  return sys;
}
} // namespace

TEST_CASE("Clock enable", "[scope:core][scope:core.sim][kind:unit][arch:*]") {
  std::unique_ptr<System> sys;
  StateRegister *ctl = nullptr;
  ScheduleCounter counter;
  auto clock = [&](std::string_view name) { return sys->find_absolute(name)->capability<ClockSource>(); };
  auto enabled = [&](std::string_view name) { return clock(name)->schedule().enabled; };
  auto build = [&](const std::string &clk_enable, const std::string &child_enable = "null", u8 fill = 0) {
    sys = make_system(clk_enable, child_enable, fill);
    ctl = dynamic_cast<StateRegister *>(sys->find_absolute("/ctl"));
    dynamic_cast<EventSource *>(clock("/clk"))->subscribe(&counter);
  };
  auto write = [&](Address address, u8 value, Operation::Type type = Operation::Type::Standard) {
    ctl->write<u8>(address, value, Operation{type, Operation::Kind::data});
  };

  SECTION("disable_on_write latches off on any program write until reset") {
    build(R"({"source": "/ctl", "mode": "disable_on_write"})");
    CHECK(enabled("/clk"));
    write(0, 1, Operation::Type::Application);
    CHECK(enabled("/clk"));
    write(0, 0);
    CHECK(!enabled("/clk"));
    CHECK(!enabled("/child"));
    write(1, 1);
    CHECK(counter.count == 1);
    sys->find_absolute("/clk")->reset();
    CHECK(enabled("/clk"));
  }
  SECTION("enable_when performs a memory comparison") {
    build(R"({"source": "/ctl", "mode": "enable_when", "mask": "0x00F0", "match": "0x0050", "order": "big"})", "null",
          0x5A);
    CHECK(enabled("/clk"));
    // Little-endian would read 0x5A00 here and disable the clock.
    write(0, 0x00);
    CHECK(enabled("/clk"));
    write(1, 0x60);
    CHECK(!enabled("/clk"));
  }
  SECTION("Children can be disabled when parent is still enabled") {
    build(R"({"source": "/ctl", "mode": "enable_when", "offset": 0})",
          R"({"source": "/ctl", "mode": "enable_when", "offset": 1})");
    write(0, 1);
    CHECK(enabled("/clk"));
    CHECK(!enabled("/child"));
    write(1, 1);
    CHECK(enabled("/child"));
    // /clk ignores writes outside its watched byte.
    CHECK(counter.count == 1);
    write(0, 0);
    CHECK(!enabled("/child"));
  }
  SECTION("settle() synchronizes clock state to memory") {
    build(R"({"source": "/ctl", "mode": "enable_when", "offset": 0})",
          R"({"source": "/ctl", "mode": "disable_on_write", "offset": 1})");
    write(0, 1, Operation::Type::Application);
    CHECK(!enabled("/clk"));
    sys->settle();
    CHECK(enabled("/clk"));
    CHECK(counter.count == 1);
    write(1, 1);
    CHECK(!enabled("/child"));
    sys->settle();
    CHECK(!enabled("/child"));
    sys->reset();
    CHECK(dynamic_cast<pepp::ClockNode *>(sys->find_absolute("/child"))->self_enabled());
  }
  SECTION("MuxClock with a selection change") {
    build("null");
    auto *mux = dynamic_cast<pepp::MuxClock *>(sys->find_absolute("/mux"));
    mux->subscribe(&counter);
    mux->select_clock(0);
    CHECK(counter.count == 0);
    mux->select_clock(1);
    CHECK(counter.count == 1);
    CHECK(mux->schedule().period == 20);
  }
}
