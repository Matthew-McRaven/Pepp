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

#include "./instr/api.hpp"
#include "core/sim/debugger/register_blaster.hpp"
#include "core/sim/debugger/tvm_tracebuffer.hpp"

TEST_CASE("Access registers from RegisterBlaster", "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  using namespace tvm::EncodedOp;
  using SP = tvm::SegmentPair;
  auto [sys, mem, cpu] = make_cpu(PepISA3CPU::ISA::Pep10);
  auto bufmgr = sys->buffer_manager();
  tvm::TraceBuffer tb(bufmgr, 1);
  constexpr u16 S = 0; // submitter id

  // Helpers to reduce encode-then-emit boilerplate.
  auto prefix = [&](auto enc) { tb.emit_prefix(S, {enc.data(), enc.size()}); };
  auto body = [&](auto enc) { tb.emit_body(S, {enc.data(), enc.size()}); };

  SECTION("Validate that a system-created RegisterBlaster works") {
    auto blaster = sys->make_blaster();
    auto before = tb.cursor();

    tb.begin(S);
    body(LDMOD1Lo{0x1234}.encode());
    // HALT is appended automatically by end().
    auto loc = tb.end(S);

    CHECK(blaster->csrs().L == 1);
    CHECK(blaster->csrs().M1 == 0);
    CHECK(blaster->regs().MOD1.lo == 0);
    blaster->run_direct(loc);
    CHECK(blaster->csrs().L == 0);
    CHECK(blaster->csrs().M1 == 1);
    CHECK(blaster->regs().MOD1.lo == 0x1234);
  }

  SECTION("Compare accumulator (immediate)") {
    auto blaster = sys->make_blaster();
    auto scan = sys->register_scan();
    cpu->write_register(isa::Pep10::Register::A, 0xFEED);
    auto ref = *scan->find("A");

    auto before = tb.cursor();
    tb.begin(S);
    body(CmpReg<3>(ref.reg.value, ref.field.value).encode(0xFEED));
    auto loc = tb.end(S);

    CHECK(blaster->csrs().L == 1);
    CHECK(blaster->csrs().F == 0);
    CHECK(blaster->csrs().Z == 0);
    blaster->run_direct(loc);
    CHECK(blaster->csrs().L == 0);
    CHECK(blaster->csrs().F == 0);
    CHECK(blaster->csrs().Z == 1);
  }

  SECTION("Compare accumulator / X (DP)") {
    auto blaster = sys->make_blaster();
    auto scan = sys->register_scan();
    cpu->write_register(isa::Pep10::Register::A, 0xFEED);
    cpu->write_register(isa::Pep10::Register::X, 0xBEEF);
    auto a = *scan->find("A");
    auto x = *scan->find("X");

    auto before = tb.cursor();
    // Program 1: compare A to 0xFEED via DP-relative data.
    tb.begin(S);
    // Data is LE: 0xFEED => {0xED, 0xFE}
    auto dhead = tb.append_data(S, std::array<u8, 2>{0xED, 0xFE});
    prefix(LDP<3>(SP{.hi = (u16)dhead.id.value, .lo = dhead.offset}, 2).encode());
    body(CmpReg<2>(a.reg.value, a.field.value).encode());
    tb.end(S);

    // Program 2: compare X to 0xBEEF. DP retained from program 1; use ACCDP.
    tb.begin(S);
    tb.append_data(S, std::array<u8, 2>{0xEF, 0xBE});
    prefix(ACCDP{2}.encode());
    body(CmpReg<2>(x.reg.value, x.field.value).encode());
    tb.end(S);

    CHECK(blaster->csrs().L == 1);
    CHECK(blaster->csrs().F == 0);
    CHECK(blaster->csrs().Z == 0);

    auto r = tb.range(before, tb.cursor());
    auto it = r.begin();
    // Program 1
    blaster->run_direct(*it);
    CHECK(blaster->csrs().L == 0);
    CHECK(blaster->csrs().F == 0);
    CHECK(blaster->csrs().Z == 1);
    // Force-clear Z to ensure program 2 sets it independently.
    blaster->csrs().Z = 0;
    ++it;
    // Program 2
    blaster->run_direct(*it);
    CHECK(blaster->csrs().L == 0);
    CHECK(blaster->csrs().F == 0);
    CHECK(blaster->csrs().Z == 1);
  }

  SECTION("Set, compare, and clear memory") {
    auto blaster = sys->make_blaster();
    const u32 offset = 0xFEED;
    const u16 val = 0xBEEF;

    auto before = tb.cursor();

    // Program 1: set memory, then compare.
    tb.begin(S);
    auto dhead = tb.append_data(S, std::array<u8, 2>{0xBE, 0xEF});
    prefix(LDP<3>(SP{.hi = (u16)dhead.id.value, .lo = dhead.offset}, 2).encode());
    body(SetMem<false, 4>{.access = rw.as_u8(), .dev = mem->id().value, .off = SP{.hi = 0, .lo = (u16)offset}}
             .encode());
    body(CmpMem<3>{.dev = mem->id().value, .off = SP{.hi = 0, .lo = (u16)offset}}.encode());
    tb.end(S);

    // Program 2: clear memory.
    tb.begin(S);
    body(ClrMem<1>{.dev = mem->id().value}.encode());
    tb.end(S);

    CHECK(blaster->csrs().L == 1);
    CHECK(blaster->csrs().F == 0);
    CHECK(blaster->csrs().Z == 0);
    CHECK(((Target *)mem)->read<u16, bits::host_is_le>(offset, rw).second == 0x0000);

    auto r = tb.range(before, tb.cursor());
    auto it = r.begin();
    // Program 1: set + compare
    blaster->run_direct(*it);
    CHECK(blaster->csrs().L == 0);
    CHECK(blaster->csrs().F == 0);
    CHECK(blaster->csrs().Z == 1);
    CHECK(((Target *)mem)->read<u16, bits::host_is_le>(offset, rw).second == val);
    // Program 2: clear
    ++it;
    blaster->run_direct(*it);
    CHECK(blaster->csrs().L == 0);
    CHECK(blaster->csrs().F == 0);
    CHECK(((Target *)mem)->read<u16, bits::host_is_le>(offset, rw).second == 0x0000);
  }
}

namespace {

// A 32-bit value, its little-endian byte order (TVM-required order), and its big-endian order (Pep/N required order).
constexpr u32 WIDE_VALUE = 0x1122'3344;
constexpr std::array<u8, 4> WIDE_LE{0x44, 0x33, 0x22, 0x11};
constexpr std::array<u8, 4> WIDE_BE{0x11, 0x22, 0x33, 0x44};
constexpr Address WIDE_OFFSET = 0x10;

// Create a "fake" 4-byte register inside pepp for testing purposes.
RegisterScan::RegisterRef expose_wide(System &sys, Dense &mem) {
  RegisterScan::Register wide{};
  wide.order = bits::Order::BigEndian;
  wide.byte_width = 4;
  wide.access = RegisterScan::Register::ReadWrite;
  wide.target = mem.id();
  wide.offset = WIDE_OFFSET;
  wide.name = "WIDE";
  sys.register_scan()->expose(wide);
  mem.write(WIDE_OFFSET, {WIDE_BE.data(), WIDE_BE.size()}, rw);
  return *sys.register_scan()->find("WIDE");
}

} // namespace

TEST_CASE("Expose a 4-byte register", "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  auto [sys, mem, cpu] = make_cpu(PepISA3CPU::ISA::Pep10);
  auto ref = expose_wide(*sys, *mem);
  // The scan reads the register as a host-order u32, which is the value a compare has to match.
  CHECK(sys->register_scan()->read<u32>(ref) == WIDE_VALUE);
}

// The 4-byte case has to assemble its two halves little-endian, matching its own 1- and 2-byte cases (see the LE data
// in "Compare accumulator / X (DP)" above) and every other immediate in the ISA.
TEST_CASE("CMPREG compares a 4-byte register little-endian",
          "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  using namespace tvm::EncodedOp;
  constexpr u16 S = 0;
  auto [sys, mem, cpu] = make_cpu(PepISA3CPU::ISA::Pep10);
  auto ref = expose_wide(*sys, *mem);
  auto blaster = sys->make_blaster();
  tvm::TraceBuffer tb(sys->buffer_manager(), 1);

  // Expected value supplied little-endian. Under the current assembly order this reads as 0x33441122 instead.
  auto enc = CmpReg<3>(ref.reg.value, ref.field.value).encode(WIDE_LE);
  tb.begin(S);
  tb.emit_body(S, {enc.data(), enc.size()});
  auto loc = tb.end(S);

  blaster->run_direct(loc);

  CHECK(blaster->stop_cause() == StopCause::None);
  CHECK(blaster->csrs().Z == 1);
  CHECK(blaster->csrs().N == 0);
}
