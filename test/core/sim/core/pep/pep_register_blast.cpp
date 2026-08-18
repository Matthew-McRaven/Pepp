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

#include "./instr/api.hpp"
#include "core/sim/debugger/tvm_interpreter.hpp"
#include "core/sim/debugger/tvm_tracebuffer.hpp"

TEST_CASE("Access registers from tvm::Interpreter", "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  using namespace tvm::EncodedOp;
  using SP = tvm::SegmentPair;
  auto [sys, mem, cpu] = make_cpu(PepISA3CPU::ISA::Pep10);
  auto bufmgr = sys->buffer_manager();
  tvm::TraceBuffer tb(bufmgr);
  constexpr Device::ID S{1}; // initiator id

  // Helpers to reduce encode-then-emit boilerplate.
  auto prefix = [&](auto enc) { tb.emit_prefix(S, {enc.data(), enc.size()}); };
  auto body = [&](auto enc) { tb.emit_body(S, {enc.data(), enc.size()}); };

  SECTION("Validate that a system-created tvm::Interpreter works") {
    auto blaster = sys->make_trace_interpreter();

    tb.begin(S);
    body(LDMOD1Lo{0x1234}.encode());
    // HALT is appended automatically by end().
    auto loc = tb.commit(S);

    CHECK(blaster->csrs().L == 1);
    CHECK(blaster->csrs().M1 == 0);
    CHECK(blaster->regs().MOD1.lo == 0);
    blaster->run(loc);
    CHECK(blaster->csrs().L == 0);
    CHECK(blaster->csrs().M1 == 1);
    CHECK(blaster->regs().MOD1.lo == 0x1234);
  }

  SECTION("Compare accumulator (immediate)") {
    auto blaster = sys->make_trace_interpreter();
    auto scan = sys->register_scan();
    cpu->write_register(isa::Pep10::Register::A, 0xFEED);
    auto ref = *scan->find("A");

    tb.begin(S);
    body(CmpReg<3>(ref.reg.value, ref.field.value).encode(0xFEED));
    auto loc = tb.commit(S);

    CHECK(blaster->csrs().L == 1);
    CHECK(blaster->csrs().F == 0);
    CHECK(blaster->csrs().Z == 0);
    blaster->run(loc);
    CHECK(blaster->csrs().L == 0);
    CHECK(blaster->csrs().F == 0);
    CHECK(blaster->csrs().Z == 1);
  }

  SECTION("Compare accumulator / X (DP)") {
    auto blaster = sys->make_trace_interpreter();
    auto scan = sys->register_scan();
    cpu->write_register(isa::Pep10::Register::A, 0xFEED);
    cpu->write_register(isa::Pep10::Register::X, 0xBEEF);
    auto a = *scan->find("A");
    auto x = *scan->find("X");

    auto before = tb.cursor();
    // Program 1: compare A to 0xFEED via DP-relative data. Both chunks are appended during this record, so program 2
    // carries no payload of its own -- which is what leaves DP where program 1 put it. A record that does carry one
    // has DP seeded from its location-buffer entry before it runs, and would not exercise ACCDP's retention.
    tb.begin(S);
    // Data is LE: 0xFEED => {0xED, 0xFE}
    auto dhead = tb.append_data(S, std::array<u8, 2>{0xED, 0xFE});
    tb.append_data(S, std::array<u8, 2>{0xEF, 0xBE});
    prefix(LDP<3>(SP{.hi = (u16)dhead.id.value, .lo = dhead.offset}, 2).encode());
    body(CmpReg<2>(a.reg.value, a.field.value).encode());
    tb.commit(S);

    // Program 2: compare X to 0xBEEF. DP retained from program 1; use ACCDP.
    tb.begin(S);
    prefix(ACCDP{2}.encode());
    body(CmpReg<2>(x.reg.value, x.field.value).encode());
    tb.commit(S);

    CHECK(blaster->csrs().L == 1);
    CHECK(blaster->csrs().F == 0);
    CHECK(blaster->csrs().Z == 0);

    auto r = tb.range(before, tb.cursor());
    auto it = r.begin();
    // Program 1
    blaster->run(*it);
    CHECK(blaster->csrs().L == 0);
    CHECK(blaster->csrs().F == 0);
    CHECK(blaster->csrs().Z == 1);
    // Force-clear Z to ensure program 2 sets it independently.
    blaster->csrs().Z = 0;
    ++it;
    // Program 2
    blaster->run(*it);
    CHECK(blaster->csrs().L == 0);
    CHECK(blaster->csrs().F == 0);
    CHECK(blaster->csrs().Z == 1);
  }

  SECTION("Set, compare, and clear memory") {
    auto blaster = sys->make_trace_interpreter();
    const u32 offset = 0xFEED;
    const u16 val = 0xBEEF;

    auto before = tb.cursor();

    // Program 1: set memory, then compare.
    tb.begin(S);
    auto dhead = tb.append_data(S, std::array<u8, 2>{0xBE, 0xEF});
    prefix(LDP<3>(SP{.hi = (u16)dhead.id.value, .lo = dhead.offset}, 2).encode());
    body(SetMem<false, 4>{.access = rw.as_u16(), .dev = mem->id().value, .off = SP{.hi = 0, .lo = (u16)offset}}
             .encode());
    body(CmpMem<3>{.dev = mem->id().value, .off = SP{.hi = 0, .lo = (u16)offset}}.encode());
    tb.commit(S);

    // Program 2: clear memory.
    tb.begin(S);
    body(ClrMem<1>{.dev = mem->id().value}.encode());
    tb.commit(S);

    CHECK(blaster->csrs().L == 1);
    CHECK(blaster->csrs().F == 0);
    CHECK(blaster->csrs().Z == 0);
    CHECK(((Target *)mem)->read<u16, bits::host_is_le>(offset, rw).second == 0x0000);

    auto r = tb.range(before, tb.cursor());
    auto it = r.begin();
    // Program 1: set + compare
    blaster->run(*it);
    CHECK(blaster->csrs().L == 0);
    CHECK(blaster->csrs().F == 0);
    CHECK(blaster->csrs().Z == 1);
    CHECK(((Target *)mem)->read<u16, bits::host_is_le>(offset, rw).second == val);
    // Program 2: clear
    ++it;
    blaster->run(*it);
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
  wide.guest_access = RegisterScan::Register::ReadWrite;
  wide.target = mem.id();
  wide.loc = WIDE_OFFSET;
  wide.name = "WIDE";
  sys.register_scan()->expose(wide);
  mem.write(WIDE_OFFSET, {WIDE_BE.data(), WIDE_BE.size()}, rw);
  return *sys.register_scan()->find("WIDE");
}

// The Pep cores only ever expose 1- and 2-byte big-endian registers, so the rest of RegisterScan's shape space is
// only reachable by declaring registers by hand over main memory.
struct Synthetic {
  const char *name;
  bits::Order order;
  u8 byte_width;
  Address offset;
};

constexpr Synthetic SYNTHETICS[] = {
    {"be1", bits::Order::BigEndian, 1, 0x100},    {"be2", bits::Order::BigEndian, 2, 0x110},
    {"be4", bits::Order::BigEndian, 4, 0x120},    {"le1", bits::Order::LittleEndian, 1, 0x130},
    {"le2", bits::Order::LittleEndian, 2, 0x140}, {"le4", bits::Order::LittleEndian, 4, 0x150},
};

void expose_synthetics(System &sys, Dense &mem) {
  for (const auto &s : SYNTHETICS) {
    RegisterScan::Register r{};
    r.order = s.order;
    r.byte_width = s.byte_width;
    r.guest_access = RegisterScan::Register::ReadWrite;
    r.target = mem.id();
    r.loc = s.offset;
    r.name = s.name;
    sys.register_scan()->expose(r);
  }
}

// Raw bytes as they actually sit in the target, bypassing the scan entirely -- otherwise a byte-order bug in write
// could be masked by the matching bug in read.
std::vector<u8> peek(Dense &mem, Address offset, size_t count) {
  std::vector<u8> out(count);
  mem.read(offset, {out.data(), out.size()}, rw);
  return out;
}

// The bytes `value` should occupy in a register of this width and order, spelled out by hand rather than via
// memcpy_endian so the expectation is independent of the machinery under test.
std::vector<u8> expected_bytes(u64 value, bits::Order order, size_t width) {
  std::vector<u8> out(width);
  for (size_t i = 0; i < width; ++i) {
    const u8 byte = (u8)(value >> (8 * i)); // low-order byte first
    out[order == bits::Order::BigEndian ? width - 1 - i : i] = byte;
  }
  return out;
}

// One value per width, with distinct non-zero bytes so a swapped or truncated store shows up in the bytes instead of
// hiding behind a symmetric pattern.
u64 value_for(u8 width) {
  switch (width) {
  case 1: return 0xAB;
  case 2: return 0xABCD;
  default: return 0xABCD'1234;
  }
}

// A register the scan refuses ordinary writes to. Clearing is a reset rather than a program write, so it is expected
// to go through anyway.
constexpr Address READONLY_OFFSET = 0x160;
RegisterScan::RegisterRef expose_readonly(System &sys, Dense &mem) {
  RegisterScan::Register r{};
  r.order = bits::Order::BigEndian;
  r.byte_width = 4;
  r.guest_access = RegisterScan::Register::Access::Read;
  r.target = mem.id();
  r.loc = READONLY_OFFSET;
  r.name = "ro4";
  sys.register_scan()->expose(r);
  return *sys.register_scan()->find("ro4");
}

// Dispatch to the integral write<I> overload whose width matches the register.
void write_integral(RegisterScan &scan, const RegisterScan::RegisterRef &ref, u8 width, u64 value) {
  switch (width) {
  case 1: scan.write<u8>(ref, (u8)value); break;
  case 2: scan.write<u16>(ref, (u16)value); break;
  default: scan.write<u32>(ref, (u32)value); break;
  }
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
TEST_CASE("CMPREG compares a 4-byte register little-endian", "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  using namespace tvm::EncodedOp;
  constexpr Device::ID S{1};
  auto [sys, mem, cpu] = make_cpu(PepISA3CPU::ISA::Pep10);
  auto ref = expose_wide(*sys, *mem);
  auto blaster = sys->make_trace_interpreter();
  tvm::TraceBuffer tb(sys->buffer_manager());

  // Expected value supplied little-endian. Under the current assembly order this reads as 0x33441122 instead.
  auto enc = CmpReg<3>(ref.reg.value, ref.field.value).encode(WIDE_LE);
  tb.begin(S);
  tb.emit_body(S, {enc.data(), enc.size()});
  auto loc = tb.commit(S);

  blaster->run(loc);

  CHECK(blaster->stop_cause() == StopCause::None);
  CHECK(blaster->csrs().Z == 1);
  CHECK(blaster->csrs().N == 0);
}

TEST_CASE("Write synthetic registers", "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  auto [sys, mem, cpu] = make_cpu(PepISA3CPU::ISA::Pep10);
  expose_synthetics(*sys, *mem);
  auto *scan = sys->register_scan();

  // Wider than any register under test, with distinct non-zero bytes throughout.
  constexpr u64 BIG = 0x1122'3344'5566'7788ULL;

  SECTION("Integral write lands in the register's byte order") {
    for (const auto &s : SYNTHETICS) {
      INFO("register " << s.name);
      auto ref = *scan->find(s.name);
      const u64 v = value_for(s.byte_width);

      write_integral(*scan, ref, s.byte_width, v);

      CHECK(peek(*mem, s.offset, s.byte_width) == expected_bytes(v, s.order, s.byte_width));
      CHECK(scan->read<u64>(ref) == v);
    }
  }

  SECTION("Span write with Byteswap::Never treats the source as register order") {
    for (const auto &s : SYNTHETICS) {
      INFO("register " << s.name);
      auto ref = *scan->find(s.name);
      const u64 v = value_for(s.byte_width);
      auto src = expected_bytes(v, s.order, s.byte_width);

      scan->write(ref, {src.data(), src.size()}, RegisterScan::Byteswap::Never);

      CHECK(peek(*mem, s.offset, s.byte_width) == src); // stored verbatim, no conversion
      CHECK(scan->read<u64>(ref) == v);
    }
  }

  SECTION("Span write with Byteswap::IfHostMismatch treats the source as host order") {
    for (const auto &s : SYNTHETICS) {
      INFO("register " << s.name);
      auto ref = *scan->find(s.name);
      const u64 v = value_for(s.byte_width);
      // Built for whatever the host happens to be, so the assertion holds on either endianness.
      auto src = expected_bytes(v, bits::hostOrder(), s.byte_width);

      scan->write(ref, {src.data(), src.size()}, RegisterScan::Byteswap::IfHostMismatch);

      CHECK(peek(*mem, s.offset, s.byte_width) == expected_bytes(v, s.order, s.byte_width));
      CHECK(scan->read<u64>(ref) == v);
    }
  }

  SECTION("An oversized integral source keeps the low-order bytes") {
    for (const auto &s : SYNTHETICS) {
      INFO("register " << s.name);
      auto ref = *scan->find(s.name);
      // Truncation drops high-order bytes, like a narrowing integer cast. Which end of memory that is depends on the
      // register's order, and getting it backwards would store 0x11 rather than 0x88.
      const u64 kept = BIG & ((1ULL << (8 * s.byte_width)) - 1);

      scan->write<u64>(ref, BIG);

      CHECK(peek(*mem, s.offset, s.byte_width) == expected_bytes(kept, s.order, s.byte_width));
      CHECK(scan->read<u64>(ref) == kept);
    }
  }

  SECTION("An oversized span source keeps the low-order bytes") {
    for (const auto &s : SYNTHETICS) {
      INFO("register " << s.name);
      auto ref = *scan->find(s.name);
      const u64 kept = BIG & ((1ULL << (8 * s.byte_width)) - 1);
      auto src = expected_bytes(BIG, s.order, sizeof(u64));

      scan->write(ref, {src.data(), src.size()}, RegisterScan::Byteswap::Never);

      CHECK(peek(*mem, s.offset, s.byte_width) == expected_bytes(kept, s.order, s.byte_width));
      CHECK(scan->read<u64>(ref) == kept);
    }
  }

  SECTION("Widening into a larger register follows the source's signedness") {
    // Converting to u64 is modular rather than zero-extending, so a negative source arrives as its sign-extended
    // pattern while an unsigned source of the same width does not. That only shows when the register is wider than
    // the source, hence the 4-byte registers.
    for (const char *name : {"be4", "le4"}) {
      INFO("register " << name);
      auto ref = *scan->find(name);

      scan->write<i16>(ref, -1);
      CHECK(scan->read<u32>(ref) == 0xFFFF'FFFFu);
      CHECK(scan->read<i32>(ref) == -1);

      scan->write<u16>(ref, 0xFFFF);
      CHECK(scan->read<u32>(ref) == 0x0000'FFFFu);
    }

    // Byte order is orthogonal to the extension, so confirm the sign fill reached memory in order too.
    scan->write<i16>(*scan->find("be4"), -2);
    CHECK(peek(*mem, 0x120, 4) == std::vector<u8>{0xFF, 0xFF, 0xFF, 0xFE});
    scan->write<i16>(*scan->find("le4"), -2);
    CHECK(peek(*mem, 0x150, 4) == std::vector<u8>{0xFE, 0xFF, 0xFF, 0xFF});
  }
}

TEST_CASE("NZVC fields pack independently", "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  auto [sys, mem, cpu] = make_cpu(PepISA3CPU::ISA::Pep10);
  auto *scan = sys->register_scan();

  // The four flags are 1-bit fields of one 4-byte big-endian register, one flag per byte (bit offsets 24/16/8/0).
  // Writing a field is a read-modify-write, so the interesting question is not whether the bit lands -- it is what
  // happens to every bit the write does not address.
  constexpr const char *FLAGS[] = {"N", "Z", "V", "C"};
  constexpr u32 PACKED[] = {0x0100'0000, 0x0001'0000, 0x0000'0100, 0x0000'0001};

  auto whole = *scan->find("NZVC");
  auto flag = [&](size_t i) { return *scan->find(FLAGS[i]); };

  SECTION("A flag set in isolation packs into its own byte") {
    for (size_t i = 0; i < 4; ++i) {
      INFO("setting " << FLAGS[i]);
      scan->write<u32>(whole, 0);
      scan->write<u8>(flag(i), 1);

      CHECK(scan->read<u32>(whole) == PACKED[i]);
      for (size_t j = 0; j < 4; ++j) {
        INFO("  reading " << FLAGS[j]);
        CHECK(scan->read<u8>(flag(j)) == (i == j ? 1 : 0));
      }
    }
  }

  SECTION("A flag cleared in isolation leaves its siblings set") {
    for (size_t i = 0; i < 4; ++i) {
      INFO("clearing " << FLAGS[i]);
      scan->write<u32>(whole, 0x0101'0101);
      scan->write<u8>(flag(i), 0);

      CHECK(scan->read<u32>(whole) == (0x0101'0101u & ~PACKED[i]));
      for (size_t j = 0; j < 4; ++j) {
        INFO("  reading " << FLAGS[j]);
        CHECK(scan->read<u8>(flag(j)) == (i == j ? 0 : 1));
      }
    }
  }

  SECTION("A field write touches exactly one bit of the register") {
    // Seed every bit, including the seven spare bits in each flag's byte that no field claims. A read-modify-write
    // whose mask is too wide -- or that skips the read entirely -- knocks some of them down.
    for (size_t i = 0; i < 4; ++i) {
      INFO("clearing " << FLAGS[i] << " out of an all-ones register");
      scan->write<u32>(whole, 0xFFFF'FFFF);
      scan->write<u8>(flag(i), 0);
      CHECK(scan->read<u32>(whole) == (0xFFFF'FFFFu & ~PACKED[i]));
    }
    for (size_t i = 0; i < 4; ++i) {
      INFO("setting " << FLAGS[i] << " out of an all-zeros register");
      scan->write<u32>(whole, 0);
      scan->write<u8>(flag(i), 1);
      CHECK(scan->read<u32>(whole) == PACKED[i]);
    }
  }

  SECTION("A mixed pattern round-trips through the fields") {
    scan->write<u32>(whole, 0);
    scan->write<u8>(flag(0), 1); // N
    scan->write<u8>(flag(1), 0); // Z
    scan->write<u8>(flag(2), 1); // V
    scan->write<u8>(flag(3), 0); // C

    CHECK(scan->read<u8>(flag(0)) == 1);
    CHECK(scan->read<u8>(flag(1)) == 0);
    CHECK(scan->read<u8>(flag(2)) == 1);
    CHECK(scan->read<u8>(flag(3)) == 0);
    CHECK(scan->read<u32>(whole) == 0x0100'0100);
  }
}

TEST_CASE("Clearing registers", "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  using namespace tvm::EncodedOp;
  constexpr Device::ID S{1};
  auto [sys, mem, cpu] = make_cpu(PepISA3CPU::ISA::Pep10);
  expose_synthetics(*sys, *mem);
  auto *scan = sys->register_scan();

  // Run a single-instruction program and report the blaster, so the opcode and the scan API can be checked against
  // each other rather than only against themselves.
  auto run = [&](auto enc) {
    auto blaster = sys->make_trace_interpreter();
    tvm::TraceBuffer tb(sys->buffer_manager());
    tb.begin(S);
    tb.emit_body(S, {enc.data(), enc.size()});
    auto loc = tb.commit(S);
    blaster->run(loc);
    return blaster;
  };

  SECTION("clear() zeroes a whole register") {
    for (const auto &s : SYNTHETICS) {
      INFO("register " << s.name);
      auto ref = *scan->find(s.name);
      scan->write<u64>(ref, 0xFFFF'FFFF'FFFF'FFFFULL);
      REQUIRE(scan->read<u64>(ref) != 0);

      scan->clear(ref);

      CHECK(scan->read<u64>(ref) == 0);
      CHECK(peek(*mem, s.offset, s.byte_width) == std::vector<u8>(s.byte_width, 0));
    }
  }

  SECTION("clear() zeroes one field and leaves its siblings") {
    auto whole = *scan->find("NZVC");
    scan->write<u32>(whole, 0x0101'0101);

    scan->clear(*scan->find("V"));

    CHECK(scan->read<u8>(*scan->find("V")) == 0);
    CHECK(scan->read<u8>(*scan->find("N")) == 1);
    CHECK(scan->read<u8>(*scan->find("Z")) == 1);
    CHECK(scan->read<u8>(*scan->find("C")) == 1);
    CHECK(scan->read<u32>(whole) == 0x0101'0001);
  }
  SECTION("Can inspect call_depth") {
    auto depth = *scan->find("call_depth");
    CHECK(scan->read<u16>(depth) == 0);
  }

  SECTION("Can inspect icount") {
    // Reads the counter through the scan, which is what catches a byte_width that disagrees with the storage it
    // points at -- the whole register throws otherwise.
    auto count = *scan->find("icount");
    CHECK(scan->read<u32>(count) == 0);
  }

  SECTION("CLRREG zeroes a whole register") {
    auto ref = *scan->find("be4");
    scan->write<u32>(ref, 0xDEAD'BEEF);
    REQUIRE(scan->read<u32>(ref) == 0xDEAD'BEEF);

    // A field of 0 addresses the register itself rather than one of its fields.
    auto blaster = run(ClrReg<2>{.reg = ref.reg.value, .field = 0}.encode());

    CHECK(blaster->stop_cause() == StopCause::None);
    CHECK(blaster->csrs().F == 0);
    CHECK(scan->read<u32>(ref) == 0);
  }

  SECTION("CLRREG zeroes a single field") {
    auto whole = *scan->find("NZVC");
    auto v = *scan->find("V");
    scan->write<u32>(whole, 0x0101'0101);

    auto blaster = run(ClrReg<2>{.reg = v.reg.value, .field = v.field.value}.encode());

    CHECK(blaster->stop_cause() == StopCause::None);
    CHECK(blaster->csrs().F == 0);
    CHECK(scan->read<u32>(whole) == 0x0101'0001);
  }

  SECTION("Clearing bypasses the read-only check") {
    auto ro = expose_readonly(*sys, *mem);
    constexpr std::array<u8, 4> seed{0x11, 0x22, 0x33, 0x44};
    mem->write(READONLY_OFFSET, {seed.data(), seed.size()}, rw);
    REQUIRE(scan->read<u32>(ro) == 0x1122'3344);

    // An ordinary write is still refused -- that is what makes the clear below a bypass rather than a no-op check.
    CHECK_THROWS(scan->write<u32>(ro, 0));
    CHECK(scan->read<u32>(ro) == 0x1122'3344);

    // A clear is a reset, not a program write, so it goes through regardless of the register's access bits.
    CHECK_NOTHROW(scan->clear(ro));
    CHECK(scan->read<u32>(ro) == 0);
  }

  SECTION("The two levels are permitted independently") {
    // The shape a derived counter wants: nonsense for the guest to hand-edit, but the host has to be able to put it
    // back on a step backwards.
    RegisterScan::Register r{};
    r.order = bits::Order::BigEndian;
    r.byte_width = 2;
    r.guest_access = RegisterScan::Register::Access::Read;
    r.host_access = RegisterScan::Register::ReadWrite;
    r.target = mem->id();
    r.loc = Address{0x170};
    r.name = "counter";
    scan->expose(r);
    auto ref = *scan->find("counter");

    CHECK_THROWS(scan->write<u16>(ref, 0xBEEF));
    CHECK(scan->read<u16>(ref) == 0);

    CHECK_NOTHROW(scan->write<u16>(ref, 0xBEEF, RegisterScan::Level::Host));
    CHECK(scan->read<u16>(ref) == 0xBEEF);

    // A reset is a host write, so it goes through for the same reason.
    CHECK_NOTHROW(scan->clear(ref));
    CHECK(scan->read<u16>(ref) == 0);
  }

  SECTION("A register the host may not write refuses the host") {
    // Level is a lookup, not a bypass: deny the host and even replay is refused.
    RegisterScan::Register r{};
    r.order = bits::Order::BigEndian;
    r.byte_width = 2;
    r.guest_access = RegisterScan::Register::Access::Read;
    r.host_access = RegisterScan::Register::Access::Read;
    r.target = mem->id();
    r.loc = Address{0x180};
    r.name = "frozen";
    scan->expose(r);
    auto ref = *scan->find("frozen");

    CHECK_THROWS(scan->write<u16>(ref, 1, RegisterScan::Level::Host));
    // And a reset cannot bypass it either, which is what Tracing::Monotonic would want.
    CHECK_THROWS(scan->clear(ref));
  }

  SECTION("CLRREG bypasses the read-only check") {
    auto ro = expose_readonly(*sys, *mem);
    constexpr std::array<u8, 4> seed{0x11, 0x22, 0x33, 0x44};
    mem->write(READONLY_OFFSET, {seed.data(), seed.size()}, rw);
    REQUIRE(scan->read<u32>(ro) == 0x1122'3344);

    auto blaster = run(ClrReg<2>{.reg = ro.reg.value, .field = 0}.encode());

    CHECK(blaster->stop_cause() == StopCause::None);
    CHECK(blaster->csrs().F == 0);
    CHECK(scan->read<u32>(ro) == 0);
  }
}

TEST_CASE("Setting registers", "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  using namespace tvm::EncodedOp;
  using SP = tvm::SegmentPair;
  constexpr Device::ID S{1};
  auto [sys, mem, cpu] = make_cpu(PepISA3CPU::ISA::Pep10);
  expose_synthetics(*sys, *mem);
  auto *scan = sys->register_scan();

  // Run a one-program trace and hand back the blaster so both the register state and the stop condition can be
  // inspected.
  auto run = [&](auto build) {
    auto blaster = sys->make_trace_interpreter();
    tvm::TraceBuffer tb(sys->buffer_manager());
    tb.begin(S);
    build(tb);
    auto loc = tb.commit(S);
    blaster->run(loc);
    return blaster;
  };

  // TVM payloads are little-endian regardless of the register's own order -- the same convention CMPREG's data
  // follows, and every other immediate in the ISA. Converting to the register's order is the scan's job.
  auto le = [](u64 value, size_t width) { return expected_bytes(value, bits::Order::LittleEndian, width); };

  SECTION("SETREG stores DP-relative data into a whole register") {
    for (const auto &s : SYNTHETICS) {
      INFO("register " << s.name);
      auto ref = *scan->find(s.name);
      const u64 v = value_for(s.byte_width);
      auto src = le(v, s.byte_width);

      auto blaster = run([&](tvm::TraceBuffer &tb) {
        auto d = tb.append_data(S, {src.data(), src.size()});
        auto ldp = LDP<3>(SP{.hi = (u16)d.id.value, .lo = d.offset}, (u16)src.size()).encode();
        tb.emit_prefix(S, {ldp.data(), ldp.size()});
        auto enc = SetReg<false, 3>{.access = rw.as_u16(), .reg = ref.reg.value, .field = 0}.encode();
        tb.emit_body(S, {enc.data(), enc.size()});
      });

      CHECK(blaster->stop_cause() == StopCause::None);
      CHECK(blaster->csrs().F == 0);
      CHECK(scan->read<u64>(ref) == v);
      // The payload was little-endian; memory holds it in the register's declared order.
      CHECK(peek(*mem, s.offset, s.byte_width) == expected_bytes(v, s.order, s.byte_width));
    }
  }

  SECTION("SETREG stores immediate data into a whole register") {
    for (const auto &s : SYNTHETICS) {
      INFO("register " << s.name);
      auto ref = *scan->find(s.name);
      const u64 v = value_for(s.byte_width);

      auto blaster = run([&](tvm::TraceBuffer &tb) {
        auto imm = SetReg<false, 4>{.access = rw.as_u16(), .reg = ref.reg.value, .field = 0};
        // Immediate payload is sized by the size word the encoder emits, so match the register's width exactly.
        switch (s.byte_width) {
        case 1: {
          auto enc = imm.encode(std::array<u8, 1>{(u8)v});
          tb.emit_body(S, {enc.data(), enc.size()});
        } break;
        case 2: {
          auto enc = imm.encode(std::array<u8, 2>{(u8)v, (u8)(v >> 8)});
          tb.emit_body(S, {enc.data(), enc.size()});
        } break;
        default: {
          auto enc = imm.encode(std::array<u8, 4>{(u8)v, (u8)(v >> 8), (u8)(v >> 16), (u8)(v >> 24)});
          tb.emit_body(S, {enc.data(), enc.size()});
        } break;
        }
      });

      CHECK(blaster->stop_cause() == StopCause::None);
      CHECK(blaster->csrs().F == 0);
      CHECK(scan->read<u64>(ref) == v);
    }
  }

  SECTION("SETREGX exclusive-ors against the register's current contents") {
    for (const auto &s : SYNTHETICS) {
      INFO("register " << s.name);
      auto ref = *scan->find(s.name);
      const u64 mask = s.byte_width == 8 ? ~0ULL : (1ULL << (8 * s.byte_width)) - 1;
      const u64 before = 0xF0F0'F0F0'F0F0'F0F0ULL & mask;
      const u64 patch = 0x3333'3333'3333'3333ULL & mask;
      scan->write<u64>(ref, before);
      // Without this, a seed that failed to land would leave 0 in the register, and 0 ^ patch == patch is exactly
      // what a missing xor also produces -- the two failures would be indistinguishable.
      REQUIRE(scan->read<u64>(ref) == before);

      auto src = le(patch, s.byte_width);
      auto blaster = run([&](tvm::TraceBuffer &tb) {
        auto d = tb.append_data(S, {src.data(), src.size()});
        auto ldp = LDP<3>(SP{.hi = (u16)d.id.value, .lo = d.offset}, (u16)src.size()).encode();
        tb.emit_prefix(S, {ldp.data(), ldp.size()});
        auto enc = SetReg<true, 3>{.access = rw.as_u16(), .reg = ref.reg.value, .field = 0}.encode();
        tb.emit_body(S, {enc.data(), enc.size()});
      });

      CHECK(blaster->stop_cause() == StopCause::None);
      CHECK(blaster->csrs().F == 0);
      CHECK(scan->read<u64>(ref) == (before ^ patch));
    }
  }

  SECTION("SETREG sets a single field and leaves its siblings") {
    auto whole = *scan->find("NZVC");
    auto v = *scan->find("V");
    scan->write<u32>(whole, 0x0000'0000);

    auto blaster = run([&](tvm::TraceBuffer &tb) {
      auto enc = SetReg<false, 4>{.access = rw.as_u16(), .reg = v.reg.value, .field = v.field.value}.encode(
          std::array<u8, 4>{0x01, 0x00, 0x00, 0x00});
      tb.emit_body(S, {enc.data(), enc.size()});
    });

    CHECK(blaster->stop_cause() == StopCause::None);
    CHECK(scan->read<u8>(v) == 1);
    CHECK(scan->read<u32>(whole) == 0x0000'0100);
  }

  SECTION("A payload that is not the register's width hard stops") {
    auto ref = *scan->find("be4");
    auto blaster = run([&](tvm::TraceBuffer &tb) {
      // Two bytes of data for a four-byte register.
      auto enc = SetReg<false, 4>{.access = rw.as_u16(), .reg = ref.reg.value, .field = 0}.encode(
          std::array<u8, 2>{0xAA, 0xBB});
      tb.emit_body(S, {enc.data(), enc.size()});
    });

    CHECK(blaster->stopped());
    CHECK(blaster->csrs().F == 1);
    CHECK(blaster->stop_cause() == StopCause::RegisterSizeMismatch);
  }

  SECTION("An unknown register id hard stops") {
    auto blaster = run([&](tvm::TraceBuffer &tb) {
      auto enc =
          SetReg<false, 4>{.access = rw.as_u16(), .reg = 0xBEEF, .field = 0}.encode(std::array<u8, 2>{0xAA, 0xBB});
      tb.emit_body(S, {enc.data(), enc.size()});
    });

    CHECK(blaster->stopped());
    CHECK(blaster->csrs().F == 1);
    CHECK(blaster->stop_cause() == StopCause::RegisterInvalid);
  }
}

TEST_CASE("Comparing register fields", "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  using namespace tvm::EncodedOp;
  using SP = tvm::SegmentPair;
  constexpr Device::ID S{1};
  auto [sys, mem, cpu] = make_cpu(PepISA3CPU::ISA::Pep10);
  auto *scan = sys->register_scan();

  constexpr const char *FLAGS[] = {"N", "Z", "V", "C"};
  // Where each flag sits when the register is read as a whole -- used only to seed state, never as a payload.
  constexpr u32 PACKED[] = {0x0100'0000, 0x0001'0000, 0x0000'0100, 0x0000'0001};
  auto whole = *scan->find("NZVC");

  // Field payloads are LSB-aligned: the value naming "this flag is set" is 1, whatever the flag's bit offset in the
  // containing register happens to be. That matches what the scan hands back when reading a field.
  auto compare = [&](RegisterScan::RegisterRef ref, u32 payload) {
    std::array<u8, 4> data{(u8)payload, (u8)(payload >> 8), (u8)(payload >> 16), (u8)(payload >> 24)};
    auto blaster = sys->make_trace_interpreter();
    tvm::TraceBuffer tb(sys->buffer_manager());
    tb.begin(S);
    auto enc = CmpReg<3>(ref.reg.value, ref.field.value).encode(data);
    tb.emit_body(S, {enc.data(), enc.size()});
    auto loc = tb.commit(S);
    blaster->run(loc);
    return blaster;
  };
  auto flag = [&](size_t i) { return *scan->find(FLAGS[i]); };

  SECTION("A set field compares equal to 1 regardless of its bit offset") {
    for (size_t i = 0; i < 4; ++i) {
      INFO("flag " << FLAGS[i]);
      scan->write<u32>(whole, PACKED[i]);

      auto blaster = compare(flag(i), 1);

      CHECK(blaster->stop_cause() == StopCause::None);
      CHECK(blaster->csrs().F == 0);
      CHECK(blaster->csrs().Z == 1);
      CHECK(blaster->csrs().N == 0);
    }
  }

  SECTION("Each flag compares independently of its siblings") {
    for (size_t i = 0; i < 4; ++i) {
      INFO("only " << FLAGS[i] << " set");
      scan->write<u32>(whole, PACKED[i]);
      for (size_t j = 0; j < 4; ++j) {
        INFO("  comparing " << FLAGS[j]);
        CHECK(compare(flag(j), 1)->csrs().Z == (i == j ? 1 : 0));
        CHECK(compare(flag(j), 0)->csrs().Z == (i == j ? 0 : 1));
      }
    }
  }

  SECTION("The same payload means different things to a field and to the register") {
    // N and V both set. A payload of 1 matches the V field, because 1 is how an LSB-aligned field says "set". The
    // identical payload against the whole register is the number 1, which 0x01000100 is not.
    scan->write<u32>(whole, 0x0100'0100);

    CHECK(compare(flag(2), 1)->csrs().Z == 1);
    CHECK(compare(whole, 1)->csrs().Z == 0);
  }

  SECTION("A payload wider than the field is masked to the field's width") {
    // Only bit 0 belongs to a 1-bit field; the rest of the payload is excess and must not affect the result.
    scan->write<u32>(whole, PACKED[2]); // V set
    CHECK(compare(flag(2), 0xFFFF'FFFF)->csrs().Z == 1);
    CHECK(compare(flag(2), 0xFFFF'FFFE)->csrs().Z == 0);
  }

  SECTION("Field compares report ordering, not just equality") {
    scan->write<u32>(whole, PACKED[2]); // V set
    auto greater = compare(flag(2), 0); // 1 > 0
    CHECK(greater->csrs().Z == 0);
    CHECK(greater->csrs().N == 0);

    scan->write<u32>(whole, 0);      // V clear
    auto less = compare(flag(2), 1); // 0 < 1
    CHECK(less->csrs().Z == 0);
    CHECK(less->csrs().N == 1);
  }

  SECTION("A field compares the same way through DP-relative data") {
    scan->write<u32>(whole, PACKED[1]); // Z set
    auto z = flag(1);
    // Still LSB-aligned, still the register's width -- the size check is against the containing register.
    auto src = expected_bytes(1, bits::Order::LittleEndian, 4);

    auto blaster = sys->make_trace_interpreter();
    tvm::TraceBuffer tb(sys->buffer_manager());
    tb.begin(S);
    auto d = tb.append_data(S, {src.data(), src.size()});
    auto ldp = LDP<3>(SP{.hi = (u16)d.id.value, .lo = d.offset}, (u16)src.size()).encode();
    tb.emit_prefix(S, {ldp.data(), ldp.size()});
    auto enc = CmpReg<2>(z.reg.value, z.field.value).encode();
    tb.emit_body(S, {enc.data(), enc.size()});
    auto loc = tb.commit(S);
    blaster->run(loc);

    CHECK(blaster->stop_cause() == StopCause::None);
    CHECK(blaster->csrs().F == 0);
    CHECK(blaster->csrs().Z == 1);
  }
}
