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

#include "core/sim/debugger/tvm_interpreter.hpp"
#include "core/sim/debugger/tvm_tracebuffer.hpp"

namespace {

// Load a program at the start of a fresh buffer and aim the blaster at it. INVCALL's targets are absolute IP offsets
// rather than displacements, so the tests below need to know where the program starts -- which rules out going
// through the TraceBuffer, since it picks the offset itself.
pepp::bts::Buffer *load_program(pepp::bts::BufferManager &mgr, tvm::Interpreter &blaster,
                                bits::span<const u8> program) {
  auto *code = mgr.alloc_buffer();
  auto offset = code->append(program);
  blaster.update_ip(code->id(), (u16)offset);
  return code;
}

} // namespace

TEST_CASE("tvm::Interpreter: basic opcodes tests", "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  auto mgr = std::make_shared<pepp::bts::BufferManager>();
  using namespace tvm::EncodedOp;
  using M = tvm::RegMask;
  tvm::TraceBuffer tb(mgr, 1);
  constexpr u16 S = 0;

  auto prefix = [&](auto enc) { tb.emit_prefix(S, {enc.data(), enc.size()}); };
  auto body = [&](auto enc) { tb.emit_body(S, {enc.data(), enc.size()}); };

  SECTION("Can copy values into common registers") {
    tvm::Interpreter blaster(mgr);
    auto before = tb.cursor();

    tb.begin(S);
    body(LDMOD1Lo{0x1234}.encode());
    tb.end(S);

    CHECK(!blaster.stopped());
    CHECK(blaster.csrs().M1 == 0);
    CHECK(blaster.regs().MOD1.lo == 0);
    for (auto loc : tb.range(before, tb.cursor()))
      blaster.run(loc);
    CHECK(blaster.stopped());
    CHECK(blaster.csrs().M1 == 1);
    CHECK(blaster.regs().MOD1.lo == 0x1234);
  }

  SECTION("Load multiple registers") {
    tvm::Interpreter blaster(mgr);
    auto before = tb.cursor();

    tb.begin(S);
    body(LMR_of<false>(std::pair{M::MOD1_LO, u16(0x1234)}, std::pair{M::ID_HI, u16(0xFEED)},
                        std::pair{M::DP_LO, u16(0xBEEF)}));
    tb.end(S);

    CHECK(!blaster.stopped());
    CHECK(blaster.csrs().M1 == 0);
    CHECK(blaster.regs().MOD1.lo == 0);
    CHECK(blaster.regs().ID.hi == 0);
    CHECK(blaster.regs().DP.lo == 0);
    for (auto loc : tb.range(before, tb.cursor()))
      blaster.run(loc);
    CHECK(blaster.stopped());
    CHECK(blaster.stop_cause() == StopCause::None);
    CHECK(blaster.csrs().M1 == 1);
    CHECK(blaster.regs().MOD1.lo == 0x1234);
    CHECK(blaster.regs().ID.hi == 0xFEED);
    CHECK(blaster.regs().DP.lo == 0xBEEF);
  }

  // Branch tests: the body contains BR + LMR + (HALT appended by TB).
  // BR<1>(0x6) jumps over the 6-byte LMR to land on the HALT.
  SECTION("Unconditional branch!") {
    tvm::Interpreter blaster(mgr);
    auto before = tb.cursor();

    tb.begin(S);
    body(BR<1>{0x6}.encode());
    body(LMR_of<false>(std::pair{M::DP_LO, u16(0xBEEF)}));
    tb.end(S);

    CHECK(blaster.regs().DP.lo != 0xBEEF);
    for (auto loc : tb.range(before, tb.cursor()))
      blaster.run(loc);
    CHECK(blaster.regs().DP.lo != 0xBEEF);
    CHECK(blaster.stopped());
    CHECK(blaster.stop_cause() == StopCause::None);
  }

  SECTION("Conditional branch (not taken)") {
    tvm::Interpreter blaster(mgr);
    blaster.csrs().Z = 0;
    auto before = tb.cursor();

    tb.begin(S);
    body(BREQ<1>{0x6}.encode());
    body(LMR_of<false>(std::pair{M::DP_LO, u16(0xBEEF)}));
    tb.end(S);

    CHECK(blaster.regs().DP.lo != 0xBEEF);
    for (auto loc : tb.range(before, tb.cursor()))
      blaster.run(loc);
    // Branch not taken: LMR executed, DP.lo set.
    CHECK(blaster.regs().DP.lo == 0xBEEF);
    CHECK(blaster.stopped());
  }

  SECTION("Conditional branch (taken)") {
    tvm::Interpreter blaster(mgr);
    blaster.csrs().Z = 1;
    auto before = tb.cursor();

    tb.begin(S);
    body(BREQ<1>{0x6}.encode());
    body(LMR_of<false>(std::pair{M::DP_LO, u16(0xBEEF)}));
    tb.end(S);

    CHECK(blaster.regs().DP.lo != 0xBEEF);
    for (auto loc : tb.range(before, tb.cursor()))
      blaster.run(loc);
    // Branch taken: LMR skipped, DP.lo unchanged.
    CHECK(blaster.regs().DP.lo != 0xBEEF);
    CHECK(blaster.stopped());
    CHECK(blaster.stop_cause() == StopCause::None);
  }
}

TEST_CASE("tvm::Interpreter: INVCALL opcode", "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  auto mgr = std::make_shared<pepp::bts::BufferManager>();
  using namespace tvm::EncodedOp;

  // Two recognizable destinations. A single step never fetches from the target, so these only have to be
  // distinguishable, not executable.
  constexpr u16 ON_TRUE = 0x40, ON_FALSE = 0x80;

  SECTION("F set picks the true target") {
    tvm::Interpreter blaster(mgr);
    constexpr auto program = InvCall<2>{.on_true_lo = ON_TRUE, .on_false_lo = ON_FALSE}.encode();
    auto *code = load_program(*mgr, blaster, program);

    blaster.csrs().F = 1;
    blaster.step();

    CHECK(blaster.regs().IP.hi == code->id().value);
    CHECK(blaster.regs().IP.lo == ON_TRUE);
  }

  SECTION("F clear picks the false target") {
    tvm::Interpreter blaster(mgr);
    constexpr auto program = InvCall<2>{.on_true_lo = ON_TRUE, .on_false_lo = ON_FALSE}.encode();
    auto *code = load_program(*mgr, blaster, program);

    blaster.csrs().F = 0;
    blaster.step();

    CHECK(blaster.regs().IP.hi == code->id().value);
    CHECK(blaster.regs().IP.lo == ON_FALSE);
  }

  SECTION("The far form picks the whole target, buffer and all") {
    // Both halves of the selected target have to come from the same side. Getting this wrong -- mixing one target's
    // hi with the other's lo -- is exactly what the interleaved lo-first encoding invites.
    constexpr auto program = InvCall<4>{.on_true = SegmentPair{.hi = 0x1234, .lo = ON_TRUE},
                                        .on_false = SegmentPair{.hi = 0x5678, .lo = ON_FALSE}}
                                 .encode();

    tvm::Interpreter on_true(mgr);
    load_program(*mgr, on_true, program);
    on_true.csrs().F = 1;
    on_true.step();
    CHECK(on_true.regs().IP.hi == 0x1234);
    CHECK(on_true.regs().IP.lo == ON_TRUE);

    tvm::Interpreter on_false(mgr);
    load_program(*mgr, on_false, program);
    on_false.csrs().F = 0;
    on_false.step();
    CHECK(on_false.regs().IP.hi == 0x5678);
    CHECK(on_false.regs().IP.lo == ON_FALSE);
  }

  SECTION("The target is called, not branched to") {
    tvm::Interpreter blaster(mgr);
    std::vector<u8> program;
    auto append = [&](auto enc) { program.insert(program.end(), enc.begin(), enc.end()); };
    append(InvCall<2>{.on_true_lo = 8, .on_false_lo = ON_FALSE}.encode()); // bytes 0..5
    append(Halt<0>{}.encode());                                            // bytes 6..7, the return point
    append(Ret<0>{}.encode());                                             // bytes 8..9, the true target

    load_program(*mgr, blaster, program);
    blaster.csrs().F = 1;

    blaster.step(); // INVCALL
    CHECK(blaster.regs().IP.lo == 8);
    CHECK(blaster.regs().SP == 4); // a frame was pushed, so this was a call rather than a jump

    blaster.step(); // RET
    CHECK(blaster.regs().IP.lo == 6); // the pushed address is the instruction after INVCALL
    CHECK(blaster.regs().SP == 0);

    blaster.step(); // HALT
    CHECK(blaster.stopped());
    CHECK(blaster.stop_cause() == StopCause::None);
  }
}
