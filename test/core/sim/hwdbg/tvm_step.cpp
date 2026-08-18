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

#include "core/sim/debugger/tvm_interpreter.hpp"
#include "core/sim/debugger/tvm_tracebuffer.hpp"
#include "core/sim/memory/ram/dense.hpp"
#include "core/sim/system.hpp"

namespace {

// A system with nothing in it but one RAM.
auto make_system() {
  System::Configuration root_cfg{{.basename = "/", .compatible = System::compatible}};
  Dense::Configuration mem_cfg{
      Device::Configuration{.basename = "memory", .compatible = Dense::compatible},
      0x00,
      AddressSpan(0x0000, 0xffff),
  };
  auto system = std::make_unique<System>(root_cfg);
  auto *mem = system->make_device<Dense>(mem_cfg);
  system->initialize();
  return std::make_tuple(std::move(system), mem);
}

const Operation app(Operation::Type::Application, Operation::Kind::data);

// Raw bytes straight out of the target. Big-endian assembly is spelled out by hand rather than going through
// memcpy_endian, so a byte-order bug in STEPMEM cannot hide behind the same bug in the check.
u16 peek_be(Target *mem, Address at) {
  std::array<u8, 2> bytes{};
  mem->read(at, {bytes.data(), bytes.size()}, app);
  return (u16)(((u16)bytes[0] << 8) | bytes[1]);
}
void poke_be(Target *mem, Address at, u16 v) {
  const std::array<u8, 2> bytes{(u8)(v >> 8), (u8)(v & 0xFF)};
  mem->write(at, {bytes.data(), bytes.size()}, app);
}
u16 peek_le(Target *mem, Address at) {
  std::array<u8, 2> bytes{};
  mem->read(at, {bytes.data(), bytes.size()}, app);
  return (u16)(((u16)bytes[1] << 8) | bytes[0]);
}
void poke_le(Target *mem, Address at, u16 v) {
  const std::array<u8, 2> bytes{(u8)(v & 0xFF), (u8)(v >> 8)};
  mem->write(at, {bytes.data(), bytes.size()}, app);
}

// Expose a register over main memory. The Pep cores only ever declare 1- and 2-byte big-endian registers, and a
// counter is exactly the thing that wants to be wider.
RegisterScan::RegisterRef expose(System &sys, Dense &mem, const char *name, u8 byte_width, Address at,
                                 bits::Order order = bits::Order::BigEndian) {
  RegisterScan::Register r{};
  r.order = order;
  r.byte_width = byte_width;
  r.guest_access = RegisterScan::Register::ReadWrite;
  r.target = mem.id();
  r.loc = at;
  r.name = name;
  sys.register_scan()->expose(r);
  return *sys.register_scan()->find(name);
}

constexpr Device::ID S{1}; // initiator id
constexpr Address ADDR = 0x1234;

} // namespace

TEST_CASE("tvm::Interpreter: STEPMEM", "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  using namespace tvm::EncodedOp;
  using SP = tvm::SegmentPair;
  auto [sys, mem] = make_system();
  tvm::TraceBuffer tb(sys->buffer_manager());
  Target *target = mem;
  const auto off = SP{.hi = (u16)(ADDR >> 16), .lo = (u16)(ADDR & 0xFFFF)};
  const u16 access = Operation(Operation::Type::Standard, Operation::Kind::data, S).as_u16();

  auto body = [&](auto enc) { tb.emit_body(S, {enc.data(), enc.size()}); };
  // MOD1.hi: 0 leaves the destination little-endian, anything else makes it big-endian. The payload itself is always
  // little-endian, like every other immediate in this ISA.
  constexpr u16 BE = 1, LE = 0;

  SECTION("A delta carries across the bytes of its destination, and undoes itself") {
    poke_be(target, ADDR, 0x00FF);

    tb.begin(S);
    body(StepMem<6>(access, mem->id().value, off, BE).encode(std::array<u8, 2>{0x01, 0x00}));
    auto loc = tb.commit(S);

    auto blaster = sys->make_trace_interpreter();
    blaster->run(loc);
    CHECK(blaster->stop_cause() == tvm::StopCause::None);
    CHECK(blaster->csrs().F == 0);
    // A carry out of the low byte lands in the high one, which is what says the destination was treated as one
    // number rather than as two independent bytes.
    CHECK(peek_be(target, ADDR) == 0x0100);

    // Backward replay subtracts, so the same record walks the counter back.
    blaster->backend().set_direction(tvm::Direction::Backward);
    blaster->run(loc);
    CHECK(blaster->csrs().F == 0);
    CHECK(peek_be(target, ADDR) == 0x00FF);
  }

  SECTION("The delta is signed") {
    poke_be(target, ADDR, 0x0100);

    tb.begin(S);
    body(StepMem<6>(access, mem->id().value, off, BE).encode(std::array<u8, 2>{0xFF, 0xFF}));
    auto loc = tb.commit(S);

    sys->make_trace_interpreter()->run(loc);
    CHECK(peek_be(target, ADDR) == 0x00FF);
  }

  SECTION("A one-byte step touches one byte") {
    poke_be(target, ADDR, 0xFF41);

    tb.begin(S);
    // the payload's width is the destination's width.
    body(StepMem<6>(access, mem->id().value, off, BE).encode(std::array<u8, 1>{0x01}));
    auto loc = tb.commit(S);

    sys->make_trace_interpreter()->run(loc);
    // The step wrapped inside the first byte instead of borrowing the second.
    CHECK(peek_be(target, ADDR) == 0x0041);
  }

  SECTION("The destination wraps within its own width") {
    poke_be(target, ADDR, 0xFFFF);
    poke_be(target, ADDR + 2, 0x9999); // a neighbour a carry must not reach

    tb.begin(S);
    body(StepMem<6>(access, mem->id().value, off, BE).encode(std::array<u8, 2>{0x01, 0x00}));
    auto loc = tb.commit(S);

    sys->make_trace_interpreter()->run(loc);
    CHECK(peek_be(target, ADDR) == 0x0000);
    CHECK(peek_be(target, ADDR + 2) == 0x9999);
  }

  SECTION("A little-endian destination steps little-endian") {
    poke_le(target, ADDR, 0x00FF);

    tb.begin(S);
    body(StepMem<6>(access, mem->id().value, off, LE).encode(std::array<u8, 2>{0x01, 0x00}));
    auto loc = tb.commit(S);

    sys->make_trace_interpreter()->run(loc);
    CHECK(peek_le(target, ADDR) == 0x0100);
    // And the same bytes read big-endian would have been a different number entirely
    CHECK(peek_be(target, ADDR) == 0x0001);
  }

  SECTION("The delta can come from DP rather than the packet") {
    poke_be(target, ADDR, 0x0010);

    tb.begin(S);
    tb.append_data(S, std::array<u8, 2>{0x05, 0x00});
    body(LDR<tvm::RegMask::DS>{2}.encode());
    body(StepMem<5>{.access = access, .dev = mem->id().value, .off = off, .order = BE}.encode());
    auto loc = tb.commit(S);

    auto blaster = sys->make_trace_interpreter();
    blaster->run(loc);
    CHECK(blaster->stop_cause() == tvm::StopCause::None);
    CHECK(peek_be(target, ADDR) == 0x0015);
  }

  SECTION("A delta too wide for the arithmetic is refused") {
    tb.begin(S);
    // The addition runs in a u64, so nine bytes of payload have nowhere to go.
    body(StepMem<6>(access, mem->id().value, off, BE).encode(std::array<u8, 9>{0x01, 0, 0, 0, 0, 0, 0, 0, 0}));
    auto loc = tb.commit(S);

    auto blaster = sys->make_trace_interpreter();
    blaster->run(loc);
    CHECK(blaster->stop_cause() == tvm::StopCause::StepWidthIllegal);
  }
}

TEST_CASE("tvm::Interpreter: STEPREG", "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  using namespace tvm::EncodedOp;
  auto [sys, mem] = make_system();
  tvm::TraceBuffer tb(sys->buffer_manager());
  auto *scan = sys->register_scan();
  const u16 access = Operation(Operation::Type::Standard, Operation::Kind::data, S).as_u16();

  auto body = [&](auto enc) { tb.emit_body(S, {enc.data(), enc.size()}); };

  SECTION("A one-byte delta steps a four-byte counter") {
    // The register reports its own width and order
    auto ref = expose(*sys, *mem, "cycles", 4, 0x100);
    scan->write<u32>(ref, 0x0000'FFFF);

    tb.begin(S);
    body(StepReg<4>(access, ref.reg.value, ref.field.value).encode(std::array<u8, 1>{0x01}));
    auto loc = tb.commit(S);

    auto blaster = sys->make_trace_interpreter();
    blaster->run(loc);
    CHECK(blaster->stop_cause() == tvm::StopCause::None);
    CHECK(blaster->csrs().F == 0);
    CHECK(scan->read<u32>(ref) == 0x0001'0000);

    blaster->backend().set_direction(tvm::Direction::Backward);
    blaster->run(loc);
    CHECK(scan->read<u32>(ref) == 0x0000'FFFF);
  }

  SECTION("A negative delta counts down") {
    auto ref = expose(*sys, *mem, "cycles", 2, 0x100);
    scan->write<u16>(ref, 0x0100);

    tb.begin(S);
    body(StepReg<4>(access, ref.reg.value, ref.field.value).encode(std::array<u8, 1>{0xFF}));
    auto loc = tb.commit(S);

    sys->make_trace_interpreter()->run(loc);
    CHECK(scan->read<u16>(ref) == 0x00FF);
  }

  SECTION("A little-endian register steps little-endian") {
    auto ref = expose(*sys, *mem, "le", 2, 0x100, bits::Order::LittleEndian);
    scan->write<u16>(ref, 0x00FF);

    tb.begin(S);
    body(StepReg<4>(access, ref.reg.value, ref.field.value).encode(std::array<u8, 1>{0x01}));
    auto loc = tb.commit(S);

    sys->make_trace_interpreter()->run(loc);
    CHECK(scan->read<u16>(ref) == 0x0100);
    CHECK(peek_le(mem, 0x100) == 0x0100);
  }

  SECTION("A field wraps within itself and leaves its siblings alone") {
    RegisterScan::Register r{};
    r.order = bits::Order::BigEndian;
    r.byte_width = 1;
    r.guest_access = RegisterScan::Register::ReadWrite;
    r.target = mem->id();
    r.loc = Address{0x100};
    r.name = "flags";
    // Two 4-bit halves, so a carry out of the low one would be visible in the high one.
    r.fields.push_back({RegisterScan::Register::ReadWrite, RegisterScan::Register::ReadWrite, 0, 4, "lo"});
    r.fields.push_back({RegisterScan::Register::ReadWrite, RegisterScan::Register::ReadWrite, 4, 4, "hi"});
    scan->expose(r);
    auto lo = *scan->find("lo");
    auto hi = *scan->find("hi");
    scan->write<u8>(lo, 0xF);
    scan->write<u8>(hi, 0x3);

    tb.begin(S);
    body(StepReg<4>(access, lo.reg.value, lo.field.value).encode(std::array<u8, 1>{0x01}));
    auto loc = tb.commit(S);

    auto blaster = sys->make_trace_interpreter();
    blaster->run(loc);
    CHECK(blaster->csrs().F == 0);
    CHECK(scan->read<u8>(lo) == 0x0);
    CHECK(scan->read<u8>(hi) == 0x3);
  }

  SECTION("A register the scan refuses to write sets F rather than stopping") {
    RegisterScan::Register r{};
    r.order = bits::Order::BigEndian;
    r.byte_width = 2;
    // Denied to the host as well as the guest: replay is a host write, so denying only the guest would let it
    // through and there would be no refusal to observe.
    r.guest_access = RegisterScan::Register::Access::Read;
    r.host_access = RegisterScan::Register::Access::Read;
    r.target = mem->id();
    r.loc = Address{0x100};
    r.name = "ro";
    scan->expose(r);
    auto ref = *scan->find("ro");

    tb.begin(S);
    body(StepReg<4>(access, ref.reg.value, ref.field.value).encode(std::array<u8, 1>{0x01}));
    auto loc = tb.commit(S);

    // A refused register is an F for a following BRF rather than a halt -- but reaching HALT soft-stops, and that
    // clears F. So step the one instruction instead of running to completion, and read F while it still means
    // something. The payload is immediate, so this needs no DP and can skip run()'s setup entirely.
    auto blaster = sys->make_trace_interpreter();
    blaster->update_ip(loc.code);
    blaster->step();

    CHECK(!blaster->stopped()); // the machine is still live: the write failed, the program did not
    CHECK(blaster->csrs().F == 1);
    // And the refusal really did stop the write from landing.
    CHECK(scan->read<u16>(ref) == 0x0000);
  }

  SECTION("An unknown register is refused") {
    tb.begin(S);
    body(StepReg<4>(access, 0xFFFF, 0).encode(std::array<u8, 1>{0x01}));
    auto loc = tb.commit(S);

    auto blaster = sys->make_trace_interpreter();
    blaster->run(loc);
    CHECK(blaster->stop_cause() == tvm::StopCause::RegisterInvalid);
  }
}
