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
#include "core/sim/api/memory.hpp"
#include "core/sim/debugger/register_scanner.hpp"
#include "core/sim/memory/bus/simplebus.hpp"
#include "core/sim/memory/ram/dense.hpp"
#include "core/sim/system.hpp"

namespace {

using Kind = RegisterScan::Register::Kind;
using SR = RegisterScan::Register;

const Operation std_op(Operation::Type::Standard, Operation::Kind::data);

// One RAM under the root with a per-test fill
auto make_system(u8 fill) {
  System::Configuration root_cfg{{.basename = "/", .compatible = System::compatible}};
  Dense::Configuration mem_cfg{
      Device::Configuration{.basename = "memory", .compatible = Dense::compatible},
      fill,
      AddressSpan(0x0000, 0x00ff),
  };
  auto sys = std::make_unique<System>(root_cfg);
  auto *mem = sys->make_device<Dense>(mem_cfg);
  sys->initialize();
  return std::make_tuple(std::move(sys), mem);
}

// The same RAM behind a bus, for the case that needs a parent and a child rather than one flat device.
auto make_bus_system(u8 fill) {
  using Mapping = SimpleBus::Configuration::Mapping;
  System::Configuration root_cfg{{.basename = "/", .compatible = System::compatible}};
  auto sys = std::make_unique<System>(root_cfg);
  Dense::Configuration mem_cfg{
      Device::Configuration{.basename = "memory", .compatible = Dense::compatible},
      fill,
      AddressSpan(0x00, 0x0f),
  };
  auto *mem = sys->make_device<Dense>(mem_cfg);
  SimpleBus::Configuration bus_cfg{
      {Device::Configuration{.basename = "bus", .compatible = SimpleBus::compatible}},
      0,
      AddressSpan(0x00, 0x0f),
  };
  bus_cfg.mappings.push_back(Mapping{.target = mem->config().fullname, .source_span = AddressSpan(0x00, 0x0f)});
  auto *bus = sys->make_device<SimpleBus>(bus_cfg);
  sys->initialize();
  return std::make_tuple(std::move(sys), mem, bus);
}

u8 byte_at(Target *mem, Address at) {
  u8 v = 0;
  mem->read(at, {&v, 1}, std_op);
  return v;
}

void write_byte(Target *mem, Address at, u8 v) { mem->write(at, {&v, 1}, std_op); }

// A 2-byte big-endian register living at `at` inside mem, which is how the architectural registers of a CPU are
// exposed: storage owned by a Target, named by the scan.
RegisterScan::RegisterRef expose_at(RegisterScan *scan, Device::ID target, Address at, const std::string &name) {
  SR r{};
  r.byte_width = 2;
  r.order = bits::Order::BigEndian;
  r.target = target;
  r.loc = at;
  r.name = name;
  scan->expose(r);
  return *scan->find(name, target);
}

} // namespace

TEST_CASE("Device::reset", "[scope:core][scope:core.sim][kind:unit][arch:*]") {
  auto [sys, mem] = make_system(0xCD);
  auto *scan = sys->register_scan();
  write_byte(mem, 0x10, 0x11);
  REQUIRE(byte_at(mem, 0x10) == 0x11);

  SECTION("restores a memory to its configured fill") {
    mem->reset();
    CHECK(byte_at(mem, 0x10) == 0xCD);
  }

  SECTION("is reached for every device by the system walk") {
    sys->reset();
    CHECK(byte_at(mem, 0x10) == 0xCD);
  }

  SECTION("reads back a fill that clear() cannot redefine") {
    mem->clear(0x77);
    CHECK(byte_at(mem, 0x10) == 0x77);
    // The configured fill survived, so the reset that follows restores what the device was built with rather than
    // whatever the last clear happened to pass. CLRMEM replaying out of a trace is what makes that matter.
    mem->reset();
    CHECK(byte_at(mem, 0x10) == 0xCD);
  }

  SECTION("brings a device back whole, performance counters included") {
    auto rd = scan->find("rd_bytes", mem->id());
    auto wr = scan->find("wr_bytes", mem->id());
    REQUIRE(rd.has_value());
    REQUIRE(wr.has_value());
    (void)byte_at(mem, 0x10);
    REQUIRE(scan->read<u64>(*rd) > 0);
    REQUIRE(scan->read<u64>(*wr) > 0);

    // Call Dense::reset() directly to avoid the System::reset() walk.
    mem->reset();
    // Read at the counter's full width. read<u32> against a u64 register truncates rather than complaining, so a
    // reset that cleared only the low word would satisfy these.
    CHECK(scan->read<u64>(*rd) == 0);
    CHECK(scan->read<u64>(*wr) == 0);
  }

  SECTION("leaves a register backed by target memory following that memory") {
    auto ref = expose_at(scan, mem->id(), Address{0x20}, "SCRATCH");
    scan->write<u16>(ref, 0xBEEF);
    REQUIRE(scan->read<u16>(ref) == 0xBEEF);

    sys->reset();
    CHECK(scan->read<u16>(ref) == 0xCDCD);
    // Bytes the register does not cover took the same fill, which confirms the walk really ran rather than the
    // register simply never having been written.
    CHECK(byte_at(mem, 0x30) == 0xCD);

    // Zeroing it is RegisterScan::reset()'s job, a separate operation System::reset() does not perform. The same
    // register under the two mechanisms comes to rest at two different values.
    scan->reset({Kind::State});
    CHECK(scan->read<u16>(ref) == 0x0000);
  }

  SECTION("does not recurse into children, where clear() does") {
    // clear() cascades to make busses and their children look like a single device, where reset() does not.
    auto [bus_sys, bus_mem, bus] = make_bus_system(0xCD);
    write_byte(bus_mem, 0x01, 0x11);

    bus->reset();
    CHECK(byte_at(bus_mem, 0x01) == 0x11);
    bus->clear(0x22);
    CHECK(byte_at(bus_mem, 0x01) == 0x22);
  }
}

TEST_CASE("RegisterScan::reset", "[scope:core][scope:core.sim][kind:unit][arch:*]") {
  auto [sys, mem] = make_system(0x00);
  auto *scan = sys->register_scan();

  SECTION("selects on Kind, leaving machine state alone") {
    auto ref = expose_at(scan, mem->id(), Address{0x20}, "SCRATCH");
    auto rd = *scan->find("rd_bytes", mem->id());
    scan->write<u16>(ref, 0xBEEF);
    (void)byte_at(mem, 0x40);
    REQUIRE(scan->read<u64>(rd) > 0);

    const auto touched = scan->reset({Kind::Count});
    CHECK(touched == 2); // rd_bytes and wr_bytes, and no State register
    CHECK(scan->read<u64>(rd) == 0);
    // Machine state is not a statistic. A counters-only reset that also zeroed this would make measuring a run
    // impossible without destroying it.
    CHECK(scan->read<u16>(ref) == 0xBEEF);
  }

  SECTION("skips a register the host may not write rather than throwing at it") {
    SR r{};
    r.byte_width = 2;
    r.order = bits::Order::BigEndian;
    r.guest_access = SR::Access::Read;
    r.host_access = SR::Access::Read;
    r.restore_on_step_back = false;
    r.target = mem->id();
    r.loc = Address{0x20};
    r.name = "frozen";
    scan->expose(r);
    auto ref = *scan->find("frozen", mem->id());

    write_byte(mem, 0x20, 0xAB);
    write_byte(mem, 0x21, 0xCD);
    REQUIRE(scan->read<u16>(ref) == 0xABCD);

    // Call RegisterScan::reset() directly because it is unused at the System::reset() level.
    std::size_t touched = 0;
    CHECK_NOTHROW(touched = scan->reset({Kind::State}));
    // frozen is the only State register here, and it was skipped over rather than reset and counted.
    CHECK(touched == 0);
    CHECK(scan->read<u16>(ref) == 0xABCD);
  }
}
