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
#include "core/sim/memory/errors.hpp"
#include "core/sim/memory/io/state.hpp"

namespace {
const auto std_op = Operation{Operation::Type::Standard, Operation::Kind::data};

// Snapshots each MemoryWritten, including what the register holds while the event is being delivered.
struct Recording final : public EventSink {
  struct Seen {
    Device::ID from;
    const Target *target;
    AddressSpan span;
    std::vector<u8> data;
    u16 value;
  };
  void on_event(Device::ID from, const Event &event) override {
    auto *written = dynamic_cast<const MemoryWritten *>(&event);
    REQUIRE(written != nullptr);
    auto [_, value] = written->target->read<u16>(written->target->span().lower(),
                                                 Operation{Operation::Type::Application, Operation::Kind::data});
    seen.push_back({from, written->target, written->span, {written->data.begin(), written->data.end()}, value});
  }
  std::vector<Seen> seen;
};

StateRegister::Configuration two_bytes_at(Address offset, u8 fill = 0) {
  StateRegister::Configuration cfg{{.id = Device::ID{7}, .basename = "ctl", .fullname = "/ctl"}};
  cfg.span = AddressSpan(offset, offset + 1), cfg.fill = fill;
  return cfg;
}
} // namespace

TEST_CASE("StateRegister storage", "[scope:core][scope:core.sim][kind:unit][arch:*][!throws]") {
  StateRegister dev(two_bytes_at(4, 0xAA));
  Recording sink;
  auto *src = static_cast<Device &>(dev).capability<EventSource>();
  REQUIRE(src != nullptr);
  src->subscribe(&sink);

  SECTION("Starts filled") { CHECK(dev.read<u16>(4, std_op).second == 0xAAAA); }
  SECTION("Holds only the latest value") {
    dev.write<u8>(4, 1, std_op);
    dev.write<u8>(4, 2, std_op);
    CHECK(dev.read<u8>(4, std_op).second == 2);
    CHECK(dev.read<u8>(5, std_op).second == 0xAA);
  }
  SECTION("Out of bounds throws") {
    CHECK_THROWS_AS(dev.write<u8>(6, 0, std_op), Error);
    CHECK_THROWS_AS(dev.read<u16>(5, std_op), Error);
  }
  SECTION("Event delivered after value changes") {
    dev.write<u8>(5, 0x12, std_op);
    REQUIRE(sink.seen.size() == 1);
    const auto &s = sink.seen[0];
    CHECK(s.from == dev.id());
    CHECK(s.target == &dev);
    CHECK(s.span == AddressSpan(5, 5));
    CHECK(s.data == std::vector<u8>{0x12});
    CHECK(s.value == 0x12AA);
  }
  SECTION("Writes are not coalesced") {
    dev.write<u8>(4, 0, std_op);
    dev.write<u8>(4, 0, std_op);
    CHECK(sink.seen.size() == 2);
  }
  SECTION("Other write types are silent") {
    for (auto type : {Operation::Type::Application, Operation::Type::BufferInternal, Operation::Type::Speculative})
      dev.write<u8>(4, 1, Operation{type, Operation::Kind::data});
    CHECK(sink.seen.empty());
  }
  SECTION("Reads do not trigger events") {
    (void)dev.read<u16>(4, std_op);
    CHECK(sink.seen.empty());
  }
}
