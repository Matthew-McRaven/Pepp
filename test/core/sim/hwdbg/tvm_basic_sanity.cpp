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
#include <vector>

#include "core/sim/debugger/tvm_apply_backend.hpp"
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

// Same, but hands back a Location so the program can be driven through run() rather than step().
pepp::bts::Buffer::Location load_at(pepp::bts::BufferManager &mgr, bits::span<const u8> program) {
  auto *code = mgr.alloc_buffer();
  auto offset = code->append(program);
  return pepp::bts::Buffer::Location{code->id(), (u16)offset};
}

} // namespace

TEST_CASE("tvm::Interpreter: basic opcodes tests", "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  auto mgr = std::make_shared<pepp::bts::BufferManager>();
  using namespace tvm::EncodedOp;
  using M = tvm::RegMask;
  tvm::TraceBuffer tb(mgr);
  constexpr Device::ID S{1};

  auto prefix = [&](auto enc) { tb.emit_prefix(S, {enc.data(), enc.size()}); };
  auto body = [&](auto enc) { tb.emit_body(S, {enc.data(), enc.size()}); };

  SECTION("Can copy values into common registers") {
    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    auto before = tb.cursor();

    tb.begin(S);
    body(LDMOD1Lo{0x1234}.encode());
    tb.commit(S);

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
    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    auto before = tb.cursor();

    tb.begin(S);
    body(LMR_of<false>(std::pair{M::MOD1_LO, u16(0x1234)}, std::pair{M::ID_HI, u16(0xFEED)},
                        std::pair{M::DP_LO, u16(0xBEEF)}));
    tb.commit(S);

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
    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    auto before = tb.cursor();

    tb.begin(S);
    body(BR<1>{0x6}.encode());
    body(LMR_of<false>(std::pair{M::DP_LO, u16(0xBEEF)}));
    tb.commit(S);

    CHECK(blaster.regs().DP.lo != 0xBEEF);
    for (auto loc : tb.range(before, tb.cursor()))
      blaster.run(loc);
    CHECK(blaster.regs().DP.lo != 0xBEEF);
    CHECK(blaster.stopped());
    CHECK(blaster.stop_cause() == StopCause::None);
  }

  SECTION("Conditional branch (not taken)") {
    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    blaster.csrs().Z = 0;
    auto before = tb.cursor();

    tb.begin(S);
    body(BREQ<1>{0x6}.encode());
    body(LMR_of<false>(std::pair{M::DP_LO, u16(0xBEEF)}));
    tb.commit(S);

    CHECK(blaster.regs().DP.lo != 0xBEEF);
    for (auto loc : tb.range(before, tb.cursor()))
      blaster.run(loc);
    // Branch not taken: LMR executed, DP.lo set.
    CHECK(blaster.regs().DP.lo == 0xBEEF);
    CHECK(blaster.stopped());
  }

  SECTION("Conditional branch (taken)") {
    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    blaster.csrs().Z = 1;
    auto before = tb.cursor();

    tb.begin(S);
    body(BREQ<1>{0x6}.encode());
    body(LMR_of<false>(std::pair{M::DP_LO, u16(0xBEEF)}));
    tb.commit(S);

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
  using M = tvm::RegMask;
  using D = tvm::Direction;

  // Two recognizable destinations. A single step never fetches from the target, so these only have to be
  // distinguishable, not executable.
  constexpr u16 ON_FWD = 0x40, ON_BACK = 0x80;

  // Step until the machine halts, with a bound so a direction bug shows up as a failed assertion rather than a hang.
  auto run_to_halt = [](tvm::Interpreter &b) {
    for (int i = 0; i < 32 && !b.stopped(); ++i) b.step();
  };

  SECTION("Stepping forward picks the forward target") {
    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    constexpr auto program = InvCall<2>{.on_forward_lo = ON_FWD, .on_backward_lo = ON_BACK}.encode();
    auto *code = load_program(*mgr, blaster, program);

    blaster.backend().set_direction(D::Forward);
    blaster.step();

    CHECK(blaster.regs().IP.hi == code->id().value);
    CHECK(blaster.regs().IP.lo == ON_FWD);
  }

  SECTION("Stepping backward picks the backward target") {
    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    constexpr auto program = InvCall<2>{.on_forward_lo = ON_FWD, .on_backward_lo = ON_BACK}.encode();
    auto *code = load_program(*mgr, blaster, program);

    blaster.backend().set_direction(D::Backward);
    blaster.step();

    CHECK(blaster.regs().IP.hi == code->id().value);
    CHECK(blaster.regs().IP.lo == ON_BACK);
  }

  SECTION("The F bit no longer selects") {
    // F used to be the selector. A backend that still consults it would flip this program's target.
    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    constexpr auto program = InvCall<2>{.on_forward_lo = ON_FWD, .on_backward_lo = ON_BACK}.encode();
    load_program(*mgr, blaster, program);

    blaster.backend().set_direction(D::Forward);
    blaster.csrs().F = 1;
    blaster.step();

    CHECK(blaster.regs().IP.lo == ON_FWD);
    CHECK(blaster.csrs().F == 1); // and it is left alone for a following BRF
  }

  SECTION("The far form picks the whole target, buffer and all") {
    // Both halves of the selected target have to come from the same side. Getting this wrong -- mixing one target's
    // hi with the other's lo -- is exactly what the interleaved lo-first encoding invites.
    constexpr auto program = InvCall<4>{.on_forward = SegmentPair{.hi = 0x1234, .lo = ON_FWD},
                                        .on_backward = SegmentPair{.hi = 0x5678, .lo = ON_BACK}}
                                 .encode();

    tvm::Interpreter fwd(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    load_program(*mgr, fwd, program);
    fwd.backend().set_direction(D::Forward);
    fwd.step();
    CHECK(fwd.regs().IP.hi == 0x1234);
    CHECK(fwd.regs().IP.lo == ON_FWD);

    tvm::Interpreter back(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    load_program(*mgr, back, program);
    back.backend().set_direction(D::Backward);
    back.step();
    CHECK(back.regs().IP.hi == 0x5678);
    CHECK(back.regs().IP.lo == ON_BACK);
  }

  SECTION("The target is called, not branched to") {
    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    std::vector<u8> program;
    auto append = [&](auto enc) { program.insert(program.end(), enc.begin(), enc.end()); };
    append(InvCall<2>{.on_forward_lo = 8, .on_backward_lo = ON_BACK}.encode()); // bytes 0..5
    append(Halt<0>{}.encode());                                                 // bytes 6..7, the return point
    append(InvRet<0>{}.encode());                                               // bytes 8..9, the forward target

    load_program(*mgr, blaster, program);
    blaster.backend().set_direction(D::Forward);

    blaster.step(); // INVCALL
    CHECK(blaster.regs().IP.lo == 8);
    CHECK(blaster.regs().SP == 4); // a frame was pushed, so this was a call rather than a jump

    blaster.step();                   // INVRET
    CHECK(blaster.regs().IP.lo == 6); // the pushed address is the instruction after INVCALL
    CHECK(blaster.regs().SP == 0);

    blaster.step(); // HALT
    CHECK(blaster.stopped());
    CHECK(blaster.stop_cause() == StopCause::None);
  }

  // The suspension program, used from both directions. Each arm stamps a different register, so one run tells you
  // exactly which path was taken at both nesting levels.
  //
  //   0  INVCALL fwd=OUT_F bwd=OUT_B
  //   6  HALT                          <- return point
  //   8  OUT_F: ACCESS = 0xAAAA ; INVRET
  //  16  OUT_B: OFF.hi = 0xBBBB ; INVCALL fwd=IN_F bwd=IN_B ; INVRET
  //  30  IN_F:  OFF.lo = 0xCCCC ; INVRET
  //  38  IN_B:  DS     = 0xDDDD ; INVRET
  constexpr u16 OUT_F = 8, OUT_B = 16, IN_F = 30, IN_B = 38;
  auto suspension_program = [&] {
    std::vector<u8> p;
    auto append = [&](auto enc) { p.insert(p.end(), enc.begin(), enc.end()); };
    append(InvCall<2>{.on_forward_lo = OUT_F, .on_backward_lo = OUT_B}.encode()); // 0..5
    append(Halt<0>{}.encode());                                                   // 6..7
    append(LDR<M::ACCESS>{0xAAAA}.encode());                                      // 8..13
    append(InvRet<0>{}.encode());                                                 // 14..15
    append(LDR<M::OFF_HI>{0xBBBB}.encode());                                      // 16..21
    append(InvCall<2>{.on_forward_lo = IN_F, .on_backward_lo = IN_B}.encode());   // 22..27
    append(InvRet<0>{}.encode());                                                 // 28..29
    append(LDR<M::OFF_LO>{0xCCCC}.encode());                                      // 30..35
    append(InvRet<0>{}.encode());                                                 // 36..37
    append(LDR<M::DS>{0xDDDD}.encode());                                          // 38..43
    append(InvRet<0>{}.encode());                                                 // 44..45
    return p;
  }();

  SECTION("Code reached through an INVCALL runs as forward, even in a backward replay") {
    // This is the whole point of the opcode: the outer call takes its backward arm, but the nested call inside that
    // arm sees itself as forward, so a restore routine can perform the writes it needs to.
    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    load_program(*mgr, blaster, suspension_program);
    blaster.backend().set_direction(D::Backward);
    run_to_halt(blaster);

    CHECK(blaster.regs().OFF.hi == 0xBBBB); // outer took the backward arm
    CHECK(blaster.regs().OFF.lo == 0xCCCC); // inner took the FORWARD arm -- suspended
    CHECK(blaster.regs().DS != 0xDDDD);     // inner backward arm not taken
    CHECK(blaster.regs().ACCESS != 0xAAAA); // outer forward arm not taken
    CHECK(blaster.stopped());
    CHECK(blaster.stop_cause() == StopCause::None); // balanced, so HALT accepted it
    CHECK(blaster.regs().SP == 0);
  }

  SECTION("The same program forward takes only the forward arm") {
    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    load_program(*mgr, blaster, suspension_program);
    blaster.backend().set_direction(D::Forward);
    run_to_halt(blaster);

    CHECK(blaster.regs().ACCESS == 0xAAAA);
    CHECK(blaster.regs().OFF.hi != 0xBBBB);
    CHECK(blaster.stopped());
    CHECK(blaster.stop_cause() == StopCause::None);
  }

  SECTION("INVRET restores the outer direction") {
    // Two INVCALLs in sequence rather than nested: if INVRET failed to restore, the second would take the forward arm.
    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    std::vector<u8> program;
    auto append = [&](auto enc) { program.insert(program.end(), enc.begin(), enc.end()); };
    append(InvCall<2>{.on_forward_lo = 20, .on_backward_lo = 26}.encode()); // 0..5
    append(InvCall<2>{.on_forward_lo = 32, .on_backward_lo = 38}.encode()); // 6..11
    append(Halt<0>{}.encode());                                             // 12..13
    append(Halt<0>{}.encode());                                             // 14..15 (padding)
    append(Halt<0>{}.encode());                                             // 16..17 (padding)
    append(Halt<0>{}.encode());                                             // 18..19 (padding)
    append(LDR<M::ACCESS>{0x1111}.encode());                                // 20..25  first fwd
    append(LDR<M::OFF_HI>{0x2222}.encode());                                // 26..31  first bwd
    append(LDR<M::OFF_LO>{0x3333}.encode());                                // 32..37  second fwd
    append(LDR<M::DS>{0x4444}.encode());                                    // 38..43  second bwd
    // Every arm falls through into the next; give them all a shared INVRET at 44.
    append(InvRet<0>{}.encode()); // 44..45
    load_program(*mgr, blaster, program);
    blaster.backend().set_direction(D::Backward);

    blaster.step(); // INVCALL -> 26 (backward arm)
    CHECK(blaster.regs().IP.lo == 26);
    blaster.step(); // OFF.hi = 0x2222
    blaster.step(); // fall through: OFF.lo = 0x3333
    blaster.step(); // DS = 0x4444
    blaster.step(); // INVRET -> back to 6, direction restored to backward
    CHECK(blaster.regs().IP.lo == 6);
    blaster.step(); // second INVCALL, must take the BACKWARD arm again
    CHECK(blaster.regs().IP.lo == 38);
  }

  SECTION("A plain CALL inside an INVCALL does not end the suspension") {
    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    std::vector<u8> program;
    auto append = [&](auto enc) { program.insert(program.end(), enc.begin(), enc.end()); };
    // Each arm terminates itself.
    append(InvCall<2>{.on_forward_lo = 20, .on_backward_lo = 8}.encode());   // 0..5
    append(Halt<0>{}.encode());                                              // 6..7  return point
    append(Call<1>{.next_ip_lo = 26}.encode());                              // 8..11 backward arm calls a helper
    append(InvCall<2>{.on_forward_lo = 34, .on_backward_lo = 42}.encode());  // 12..17 still suspended?
    append(InvRet<0>{}.encode());                                            // 18..19 outer arm returns
    append(LDR<M::ACCESS>{0x1111}.encode());                                 // 20..25 outer forward arm (unused)
    append(LDR<M::OFF_HI>{0xB0B0}.encode());                                 // 26..31 helper body
    append(Ret<0>{}.encode());                                               // 32..33 plain RET
    append(LDR<M::OFF_LO>{0xF00D}.encode());                                 // 34..39 nested forward arm
    append(InvRet<0>{}.encode());                                            // 40..41 ...and its own INVRET
    append(LDR<M::DS>{0xDEAD}.encode());                                     // 42..47 nested backward arm
    append(InvRet<0>{}.encode());                                            // 48..49
    load_program(*mgr, blaster, program);
    blaster.backend().set_direction(D::Backward);
    run_to_halt(blaster);

    CHECK(blaster.regs().OFF.hi == 0xB0B0); // the helper ran
    // The nested INVCALL still saw itself as forward, so the plain CALL/RET round trip left the counter alone.
    CHECK(blaster.regs().OFF.lo == 0xF00D);
    CHECK(blaster.regs().DS != 0xDEAD);
    CHECK(blaster.stopped());
    CHECK(blaster.stop_cause() == StopCause::None);
  }

  SECTION("A leaked INVCALL is refused at HALT") {
    // Without the balance check the suspension would survive into every later program, silently replaying one-way ops
    // forwards during a backward walk.
    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    std::vector<u8> program;
    auto append = [&](auto enc) { program.insert(program.end(), enc.begin(), enc.end()); };
    append(InvCall<2>{.on_forward_lo = 6, .on_backward_lo = 6}.encode()); // 0..5
    append(Halt<0>{}.encode());                                           // 6..7, reached with depth still raised
    load_program(*mgr, blaster, program);
    blaster.backend().set_direction(D::Forward);
    run_to_halt(blaster);

    CHECK(blaster.stopped());
    CHECK(blaster.csrs().F == 1); // hard stop
    CHECK(blaster.stop_cause() == StopCause::UnbalancedInvCall);
  }

  SECTION("A leaked INVCALL is refused at HALT when stepping backward too") {
    // The floor is -1 here, so the leak leaves the depth at 0 -- which reads as "forward". Exactly the state that
    // would make the rest of a backward walk replay one-way ops the wrong way round if HALT let it through.
    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    std::vector<u8> program;
    auto append = [&](auto enc) { program.insert(program.end(), enc.begin(), enc.end()); };
    append(InvCall<2>{.on_forward_lo = 6, .on_backward_lo = 6}.encode()); // 0..5
    append(Halt<0>{}.encode());                                           // 6..7
    load_program(*mgr, blaster, program);
    blaster.backend().set_direction(D::Backward);
    run_to_halt(blaster);

    CHECK(blaster.stopped());
    CHECK(blaster.csrs().F == 1);
    CHECK(blaster.stop_cause() == StopCause::UnbalancedInvCall);
  }

  SECTION("An INVRET with no INVCALL is refused") {
    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    std::vector<u8> program;
    auto append = [&](auto enc) { program.insert(program.end(), enc.begin(), enc.end()); };
    append(InvRet<0>{}.encode());
    append(Halt<0>{}.encode());
    load_program(*mgr, blaster, program);
    blaster.backend().set_direction(D::Forward);
    run_to_halt(blaster);

    CHECK(blaster.stopped());
    CHECK(blaster.csrs().F == 1);
    CHECK(blaster.stop_cause() == StopCause::UnbalancedInvCall);
  }

  SECTION("An INVRET below the floor is refused in a backward replay too") {
    // -1 is the balanced floor when stepping backward. Decrementing past it would read as "forward" and corrupt the
    // rest of the walk, so it must be caught rather than wrapping.
    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    std::vector<u8> program;
    auto append = [&](auto enc) { program.insert(program.end(), enc.begin(), enc.end()); };
    append(InvRet<0>{}.encode());
    append(Halt<0>{}.encode());
    load_program(*mgr, blaster, program);
    blaster.backend().set_direction(D::Backward);
    run_to_halt(blaster);

    CHECK(blaster.stopped());
    CHECK(blaster.stop_cause() == StopCause::UnbalancedInvCall);
  }

  SECTION("Two INVCALLs closed by one INVRET are refused") {
    // Partial unwind: the counts differ by one rather than being wholly absent, which a naive "did we see any INVRET"
    // check would accept.
    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    std::vector<u8> program;
    auto append = [&](auto enc) { program.insert(program.end(), enc.begin(), enc.end()); };
    append(InvCall<2>{.on_forward_lo = 8, .on_backward_lo = 8}.encode());   // 0..5   depth 1, pushes 6
    append(Halt<0>{}.encode());                                             // 6..7
    append(InvCall<2>{.on_forward_lo = 16, .on_backward_lo = 16}.encode()); // 8..13  depth 2, pushes 14
    append(Halt<0>{}.encode());                                             // 14..15 reached with depth still 1
    append(InvRet<0>{}.encode());                                           // 16..17 depth 1, returns to 14
    load_program(*mgr, blaster, program);
    blaster.backend().set_direction(D::Forward);
    run_to_halt(blaster);

    CHECK(blaster.stopped());
    CHECK(blaster.stop_cause() == StopCause::UnbalancedInvCall);
  }

  SECTION("An extra INVRET after a balanced pair is refused") {
    // The pair balances first, so this catches an underflow that only happens after legitimate use -- the case a
    // simple "was an INVCALL ever seen" flag would miss.
    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    std::vector<u8> program;
    auto append = [&](auto enc) { program.insert(program.end(), enc.begin(), enc.end()); };
    append(InvCall<2>{.on_forward_lo = 8, .on_backward_lo = 8}.encode()); // 0..5  depth 1, pushes 6
    append(InvRet<0>{}.encode());                                         // 6..7  depth 0 == floor -> refused
    append(InvRet<0>{}.encode());                                         // 8..9  depth 0, returns to 6
    load_program(*mgr, blaster, program);
    blaster.backend().set_direction(D::Forward);
    run_to_halt(blaster);

    CHECK(blaster.stopped());
    CHECK(blaster.csrs().F == 1);
    CHECK(blaster.stop_cause() == StopCause::UnbalancedInvCall);
  }
}

TEST_CASE("tvm::Interpreter: HALT carries a stop cause", "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  auto mgr = std::make_shared<pepp::bts::BufferManager>();
  using namespace tvm::EncodedOp;

  // The cause is the packet's *first* data word. Reading the second instead -- which is what decode_halt used to do --
  // lands on the following instruction's opcode word, so the trailing HALT below makes a regression give a specific
  // wrong answer rather than whatever happened to be in an uninitialised buffer.
  std::vector<u8> program;
  auto append = [&](auto enc) { program.insert(program.end(), enc.begin(), enc.end()); };
  append(Halt<1>{StopCause::RegisterInvalid}.encode()); // 0..3, cause word at 2..3
  append(Halt<0>{}.encode());                           // 4..5, never reached

  tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
  load_program(*mgr, blaster, program);
  blaster.step();

  CHECK(blaster.stopped());
  CHECK(blaster.csrs().F == 0); // a HALT is a soft stop however it is spelled
  CHECK(blaster.stop_cause() == StopCause::RegisterInvalid);
}

TEST_CASE("tvm::Interpreter: register retention between programs",
          "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  auto mgr = std::make_shared<pepp::bts::BufferManager>();
  using namespace tvm::EncodedOp;
  using M = tvm::RegMask;
  using RR = tvm::RegisterRetention;

  // Stamps one register from each retention class, then halts.
  std::vector<u8> seed;
  auto seed_append = [&](auto enc) { seed.insert(seed.end(), enc.begin(), enc.end()); };
  seed_append(LMR_of<false>(std::pair{M::DP_HI, u16(0x0011)}, std::pair{M::DP_LO, u16(0x0022)},
                            std::pair{M::DS, u16(0x0033)}, std::pair{M::ACCESS, u16(0x0044)},
                            std::pair{M::OFF_HI, u16(0x0055)}));
  seed_append(Halt<0>{}.encode());

  // The second program stamps ID.lo, so every case below also proves restart() brought the machine back live -- a
  // machine left halted would skip this entirely and the assertion would fail rather than silently pass.
  std::vector<u8> marker;
  auto marker_append = [&](auto enc) { marker.insert(marker.end(), enc.begin(), enc.end()); };
  marker_append(LMR_of<false>(std::pair{M::ID_LO, u16(0x0099)}));
  marker_append(Halt<0>{}.encode());

  const auto seed_loc = load_at(*mgr, seed);
  const auto marker_loc = load_at(*mgr, marker);

  SECTION("All keeps everything") {
    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    blaster.run(seed_loc);
    blaster.csrs().Z = 1;
    blaster.run(marker_loc, RR::All);

    CHECK(blaster.regs().ID.lo == 0x0099); // the second program ran, so restart left the machine live
    CHECK(blaster.regs().DP.hi == 0x0011);
    CHECK(blaster.regs().DP.lo == 0x0022);
    CHECK(blaster.regs().DS == 0x0033);
    CHECK(blaster.regs().ACCESS == 0x0044);
    CHECK(blaster.regs().OFF.hi == 0x0055);
    CHECK(blaster.csrs().Z == 1); // flags survive too
  }

  SECTION("DP keeps the data pointer and drops the rest") {
    // This is run_each's default, so it is what a replay actually uses.
    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    blaster.run(seed_loc);
    blaster.csrs().Z = 1;
    blaster.run(marker_loc, RR::DP);

    CHECK(blaster.regs().ID.lo == 0x0099);
    CHECK(blaster.regs().DP.hi == 0x0011);
    CHECK(blaster.regs().DP.lo == 0x0022);
    CHECK(blaster.regs().DS == 0x0033);
    CHECK(blaster.regs().ACCESS == 0);
    CHECK(blaster.regs().OFF.hi == 0);
    CHECK(blaster.csrs().Z == 0);
  }

  SECTION("None keeps nothing") {
    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    blaster.run(seed_loc);
    blaster.csrs().Z = 1;
    blaster.run(marker_loc, RR::None);

    CHECK(blaster.regs().ID.lo == 0x0099);
    CHECK(blaster.regs().DP.hi == 0);
    CHECK(blaster.regs().DP.lo == 0);
    CHECK(blaster.regs().DS == 0);
    CHECK(blaster.regs().ACCESS == 0);
    CHECK(blaster.regs().OFF.hi == 0);
    CHECK(blaster.csrs().Z == 0);
  }

  SECTION("Every mode clears a previous hard failure") {
    // run_each decides whether to continue on the F bit, so a mode that carried F across would stop a replay dead
    // after the first failure.
    for (auto retain : {RR::All, RR::DP, RR::None}) {
      tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
      blaster.csrs().F = 1;
      blaster.run(marker_loc, retain);
      CHECK(blaster.regs().ID.lo == 0x0099);
      CHECK(blaster.csrs().F == 0);
    }
  }
}

TEST_CASE("tvm::Interpreter: stop causes", "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  auto mgr = std::make_shared<pepp::bts::BufferManager>();
  using namespace tvm::EncodedOp;

  // Not an exhaustive sweep of StopCause -- one representative of each way the machine can give up: a stack limit, a
  // decode failure, and an unreadable instruction buffer.

  SECTION("Recursing past the stack soft-stops with StackOverflow") {
    // 256-byte stack, 4 bytes per frame, so the 65th push is the one that fails.
    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    constexpr auto program = Call<1>{.next_ip_lo = 0}.encode(); // calls itself forever
    blaster.run(load_at(*mgr, program));

    CHECK(blaster.stopped());
    CHECK(blaster.csrs().F == 0); // soft: the program is malformed, but the machine is coherent
    CHECK(blaster.stop_cause() == StopCause::StackOverflow);
    CHECK(blaster.regs().SP == 256); // the failed push left it where it was
  }

  SECTION("Returning with an empty stack soft-stops with StackUnderflow") {
    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    constexpr auto program = Ret<0>{}.encode();
    load_program(*mgr, blaster, program);
    blaster.step();

    CHECK(blaster.stopped());
    CHECK(blaster.csrs().F == 0);
    CHECK(blaster.stop_cause() == StopCause::StackUnderflow);
  }

  SECTION("An unassigned opcode hard-stops with IllegalOpcode") {
    // Opcodes are 6 bits and MAX is well under 63, so anything above it decodes to nothing.
    static_assert((u8)tvm::Opcode::MAX < 63, "pick an encoding above MAX");
    const u16 word = tvm::OpWord((tvm::Opcode)((u8)tvm::Opcode::MAX + 1), true, 0).as_u16();
    const std::array<u8, 2> program{(u8)(word & 0xFF), (u8)(word >> 8)};

    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    load_program(*mgr, blaster, program);
    blaster.step();

    CHECK(blaster.stopped());
    CHECK(blaster.csrs().F == 1); // hard: there is nothing sensible to do next
    CHECK(blaster.stop_cause() == StopCause::IllegalOpcode);
  }

  SECTION("Fetching from a buffer that was never allocated hard-stops with InvalidIBuffer") {
    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    blaster.update_ip(pepp::bts::Buffer::ID{0xFFFF}, 0);
    blaster.step();

    CHECK(blaster.stopped());
    CHECK(blaster.csrs().F == 1);
    CHECK(blaster.stop_cause() == StopCause::InvalidIBuffer);
  }
}
