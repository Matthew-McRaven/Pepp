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
#include "core/sim/api/memory.hpp"
#include "core/sim/debugger/register_scanner.hpp"
#include "core/sim/memory/ram/dense.hpp"
#include "core/sim/system.hpp"

namespace {

using Kind = RegisterScan::Register::Kind;
using SR = RegisterScan::Register;
using Sel = RegisterScan::Selection;

const Operation std_op(Operation::Type::Standard, Operation::Kind::data);

// One RAM under the root. Sampling only needs a device that exposes counters.
auto make_system() {
  System::Configuration root_cfg{{.basename = "/", .compatible = System::compatible}};
  Dense::Configuration mem_cfg{
      Device::Configuration{.basename = "memory", .compatible = Dense::compatible},
      0x00,
      AddressSpan(0x0000, 0x00ff),
  };
  auto sys = std::make_unique<System>(root_cfg);
  auto *mem = sys->make_device<Dense>(mem_cfg);
  sys->initialize();
  return std::make_tuple(std::move(sys), mem);
}

u8 byte_at(Target *mem, Address at) {
  u8 v = 0;
  mem->read(at, {&v, 1}, std_op);
  return v;
}

void write_byte(Target *mem, Address at, u8 v) { mem->write(at, {&v, 1}, std_op); }

// The two counters every Dense exposes, read then write. The order is this Selection's own choice.
RegisterScan::Selection counters_of(RegisterScan *scan, Device::ID dev) {
  RegisterScan::Selection sel{scan};
  sel.add(*scan->find("rd_bytes", dev));
  sel.add(*scan->find("wr_bytes", dev));
  return sel;
}

} // namespace

TEST_CASE("RegisterScan sampling", "[scope:core][scope:core.dbg][kind:unit][arch:*][!throws]") {
  auto [sys, mem] = make_system();
  auto *scan = sys->register_scan();

  SECTION("a delta reports only the traffic between two samples") {
    const auto sel = counters_of(scan, mem->id());
    REQUIRE(sel.size() == 2);

    // Traffic before the first sample must not appear in the delta -- that is the whole reason for sampling a
    // region rather than reading a counter.
    for (int i = 0; i < 4; ++i) write_byte(mem, 0x10, 0x11);

    const auto before = sel.sample();
    // The "function" under measurement: three writes and two reads.
    for (int i = 0; i < 3; ++i) write_byte(mem, 0x20, 0x22);
    (void)byte_at(mem, 0x20);
    (void)byte_at(mem, 0x21);
    const auto after = sel.sample();

    const auto d = before.delta_to(after);
    REQUIRE(d.size() == 2);
    // rd_bytes was added first, so it is reported first.
    CHECK(d[0] == 2);
    CHECK(d[1] == 3);
  }

  SECTION("sampling does not perturb what it measures") {
    // The sampler reads at Level::Host, so its reads land as BufferInternal and are not counted.
    const auto sel = counters_of(scan, mem->id());
    const auto a = sel.sample();
    const auto b = sel.sample();
    const auto d = a.delta_to(b);
    CHECK(d[0] == 0);
    CHECK(d[1] == 0);
  }

  SECTION("two Selections built the same way are interchangeable") {
    // The hash is content identity, so rebuilding rather than retaining a Selection is not an error. Instance
    // identity would have rejected this even though the two hold the same registers in the same order.
    const auto sel = counters_of(scan, mem->id());
    const auto twin = counters_of(scan, mem->id());
    CHECK(sel.hash() == twin.hash());
    CHECK_NOTHROW(sel.sample().delta_to(twin.sample()));
  }

  SECTION("two Selections over the same registers in a different order are not") {
    const auto rd = scan->find("rd_bytes", mem->id())->reg;
    const auto wr = scan->find("wr_bytes", mem->id())->reg;
    auto forward = Sel{scan};
    REQUIRE(forward.add(rd));
    REQUIRE(forward.add(wr));
    auto reversed = Sel{scan};
    REQUIRE(reversed.add(wr));
    REQUIRE(reversed.add(rd));

    CHECK(forward.hash() != reversed.hash());
    CHECK_THROWS(forward.sample().delta_to(reversed.sample()));
  }

  SECTION("a Selection samples exactly what was added to it") {
    auto sel = Sel{scan};
    REQUIRE(sel.empty());

    const auto wr = *scan->find("wr_bytes", mem->id());
    REQUIRE(sel.add(wr));
    CHECK(sel.size() == 1);
    // A register that was never exposed is reported rather than silently skipped.
    CHECK_FALSE(sel.add(RegisterScan::Register::ID{9999}));
    // So is a field reference. Sampling one would work, but its width is in bits where a delta subtracts at a byte
    // width, so it is refused rather than quietly widened to the register it lives in.
    CHECK_FALSE(sel.add(RegisterScan::RegisterRef{wr.reg, RegisterScan::Register::Field::ID{1}}));
    CHECK(sel.size() == 1);

    const auto before = sel.sample();
    write_byte(mem, 0x10, 0x11);
    (void)byte_at(mem, 0x10);
    const auto after = sel.sample();

    // Only wr_bytes was asked for, so the read appears nowhere in the result.
    const auto d = before.delta_to(after);
    REQUIRE(d.size() == 1);
    CHECK(d[0] == 1);
  }

  SECTION("one register's change can be recovered without knowing its position") {
    const auto sel = counters_of(scan, mem->id());
    const auto rd = *scan->find("rd_bytes", mem->id());
    const auto wr = *scan->find("wr_bytes", mem->id());

    const auto before = sel.sample();
    write_byte(mem, 0x10, 0x11);
    (void)byte_at(mem, 0x10);
    (void)byte_at(mem, 0x11);
    const auto after = sel.sample();

    CHECK(sel.delta_of(before, after, rd) == 2);
    CHECK(sel.delta_of(before, after, wr) == 1);
    // And it agrees with the positional form, which is the only thing that was available before.
    const auto d = before.delta_to(after);
    CHECK(sel.delta_of(before, after, rd) == d[*sel.index_of(rd)]);
    // Values as well as changes, since "what was it at the start" is the same lookup.
    CHECK(sel.value_of(after, wr) == sel.value_of(before, wr).value() + 1);

    // A register outside the Selection is reported rather than guessed at.
    auto other = Sel{scan};
    REQUIRE(other.add(wr));
    CHECK(other.index_of(rd) == std::nullopt);
    // A Sample from a different Selection throws instead of coming back as nullopt, which would read as "that
    // register is not here" -- a wrong answer shaped like a legitimate one.
    CHECK_THROWS(other.delta_of(before, after, wr));
  }

  SECTION("a register that could not be sampled is refused at add() rather than throwing later") {
    // Each of these would throw out of sample() if it were allowed in -- on every sample, not just the first. add()
    // is where a caller can still do something about it, and it already reports failure by returning false.
    auto sel = Sel{scan};

    SR no_storage{};
    no_storage.byte_width = 2;
    no_storage.order = bits::Order::BigEndian;
    no_storage.target = mem->id();
    no_storage.name = "unbacked"; // loc left as monostate
    scan->expose(no_storage);
    CHECK_FALSE(sel.add(*scan->find("unbacked", mem->id())));

    SR unreadable{};
    unreadable.byte_width = 2;
    unreadable.order = bits::Order::BigEndian;
    unreadable.host_access = SR::Access::None;
    unreadable.restore_on_step_back = false;
    unreadable.target = mem->id();
    unreadable.loc = Address{0x20};
    unreadable.name = "write_only";
    scan->expose(unreadable);
    CHECK_FALSE(sel.add(*scan->find("write_only", mem->id())));

    // A Sample holds u64s. Anything wider would be truncated on the way in and then differenced at a width it does
    // not have, which is worse than refusing it -- the numbers would look plausible.
    SR too_wide{};
    too_wide.byte_width = 2 * sizeof(u64);
    too_wide.order = bits::Order::BigEndian;
    too_wide.target = mem->id();
    too_wide.loc = Address{0x40};
    too_wide.name = "wide";
    scan->expose(too_wide);
    CHECK_FALSE(sel.add(*scan->find("wide", mem->id())));

    CHECK(sel.empty());
    // And an empty Selection still samples, rather than being a separate broken state.
    CHECK(sel.sample().size() == 0);
  }

  SECTION("a delta is correct across a counter's wrap") {
    // While math is performed at u64, we use the register's width to mask the result and cause wrapping.
    u16 narrow = 0;
    SR r{};
    r.byte_width = sizeof(narrow);
    r.kind = Kind::Count;
    r.order = bits::hostOrder();
    r.target = mem->id();
    r.name = "narrow";
    r.loc = &narrow;
    scan->expose(r);

    auto sel = Sel{scan};
    REQUIRE(sel.add(*scan->find("narrow", mem->id())));

    narrow = 0xFFFF;
    const auto before = sel.sample();
    narrow = 0x0010; // wrapped
    const auto after = sel.sample();

    CHECK(before.delta_to(after)[0] == 0x11);
  }
}
