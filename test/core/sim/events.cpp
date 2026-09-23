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
#include <functional>
#include <vector>
#include "core/sim/api/clock.hpp"
#include "core/sim/api/event.hpp"
#include "core/sim/api/memory.hpp"

namespace {

struct TestSource final : public EventSource {
  void fire(Device::ID from, const Event &event) { raise(from, event); }
};

struct TestSink final : public EventSink {
  void on_event(Device::ID from, const Event &event) override {
    received.push_back({from, &event});
    if (then) then(event);
  }
  std::vector<std::tuple<Device::ID, const Event *>> received;
  std::function<void(const Event &)> then;
};

} // namespace

TEST_CASE("Event delivery", "[scope:core][scope:core.sim][kind:unit][arch:*]") {
  TestSource src;
  TestSink a, b;
  const Device::ID from{3};
  const u8 bytes[] = {0xAB, 0xCD};
  const auto op = Operation{Operation::Type::Standard, Operation::Kind::data};
  const MemoryWritten written(nullptr, AddressSpan(1, 2), bytes, op);

  SECTION("No-op without subscribers") { src.fire(from, written); }
  SECTION("Subscribing twice delivers once") {
    src.subscribe(&a);
    src.subscribe(&a);
    src.fire(from, UpdateSchedule{});
    CHECK(a.received.size() == 1);
    a.received.clear();
    src.unsubscribe(&a);
    src.fire(from, UpdateSchedule{});
    CHECK(a.received.empty());
    CHECK(b.received.size() == 0);
  }
}
