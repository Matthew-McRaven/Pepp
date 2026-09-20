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

#include "core/sim/debugger/tvm_apply_backend.hpp"
#include "core/sim/debugger/tvm_encoding.hpp"
#include "core/sim/debugger/tvm_interpreter.hpp"
#include "core/sim/debugger/tvm_tracebuffer.hpp"

namespace {

// Sized so that after the 2-byte tombstone and two promotions, a third body fits in the remaining space but a third
// body + its 2-byte RET does not: 2 (tombstone) + 2 * (BODY + 2) + BODY < 65536 < 2 + 2 * (BODY + 2) + BODY + 2.
// That is the boundary where ensure_capacity must roll over to a new buffer to keep body and RET contiguous.
constexpr size_t BOUNDARY_BODY = 21843;
constexpr size_t BOUNDARY_PROMOTIONS = 3;

struct StencilProbe {
  u32 hash = 0;
  pepp::bts::Buffer::ID id{}; // Target of STCALLHALT
  u16 offset = 0;
};

// Promote a number of distinct bodies as stencils.
StencilProbe promote_to_boundary(pepp::bts::BufferManager &mgr, tvm::TraceBuffer &tb,
                                  size_t promotions = BOUNDARY_PROMOTIONS) {
  constexpr Device::ID S{1};
  StencilProbe probe;
  tvm::ProgramLocation loc{};

  for (size_t i = 0; i < promotions; ++i) {
    std::vector<u8> body(BOUNDARY_BODY, static_cast<u8>(0xA0 + i));
    probe.hash = tvm::TraceBuffer::hash({body.data(), body.size()});
    for (int pass = 0; pass < 2; ++pass) {
      tb.begin(S);
      tb.emit_body(S, {body.data(), body.size()});
      loc = tb.commit(S);
    }
  }

  // With no prefix or postfix emitted, the promoted submission's program is just [STCALLHALT].
  auto *code = mgr.find(loc.code.id);
  REQUIRE(code != nullptr);
  const auto *p = code->data() + loc.code.offset;
  REQUIRE(tvm::OpWord((u16)(p[0] | (p[1] << 8))).opcode == tvm::Opcode::STCALLHALT);
  const auto target = tb.stencil_location((u16)(p[2] | (p[3] << 8)));
  probe.id = target.id;
  probe.offset = target.offset;
  return probe;
}

} // namespace

TEST_CASE("tvm::Interpreter:  Stencil promotion", "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  auto mgr = std::make_shared<pepp::bts::BufferManager>();
  using namespace tvm::EncodedOp;
  using M = tvm::RegMask;
  tvm::TraceBuffer tb(mgr);
  constexpr Device::ID S{1};

  auto body = [&](auto enc) { tb.emit_body(S, {enc.data(), enc.size()}); };

  SECTION("Short body is never promoted") {
    // LDMOD1Lo encodes to 6 bytes (3 words), which is below PROMOTION_THRESHOLD (8).
    auto short_body = LDMOD1Lo{0x1234}.encode();
    auto h = tvm::TraceBuffer::hash({short_body.data(), short_body.size()});

    CHECK(tb.stencil_count() == 0);
    CHECK(tb.pending_count() == 0);

    // Too short to promote, so it does not become ppending.
    tb.begin(S);
    body(short_body);
    tb.commit(S);

    CHECK(tb.pending_count() == 0);
    CHECK(!tb.is_pending(h));
    CHECK(!tb.is_stencil(h));
    CHECK(tb.stencil_count() == 0);

    // Re-subitted, still not pending.
    tb.begin(S);
    body(short_body);
    tb.commit(S);

    CHECK(!tb.is_stencil(h));
    CHECK(tb.stencil_count() == 0);

    // Several more submissions: still never promoted.
    for (int i = 0; i < 5; ++i) {
      tb.begin(S);
      body(short_body);
      tb.commit(S);
    }

    CHECK(!tb.is_stencil(h));
    CHECK(tb.stencil_count() == 0);
  }

  SECTION("Long body is promoted on second occurrence") {
    // LMR_of with 3 register pairs encodes to 10 bytes (5 words), above the threshold.
    auto long_body = LMR_of<false>(std::pair{M::MOD1_LO, u16(0x1234)}, std::pair{M::ID_HI, u16(0xFEED)},
                                   std::pair{M::DP_LO, u16(0xBEEF)});
    auto h = tvm::TraceBuffer::hash({long_body.data(), long_body.size()});

    CHECK(tb.stencil_count() == 0);
    CHECK(tb.pending_count() == 0);

    // First submission: enters pending set.
    tb.begin(S);
    body(long_body);
    tb.commit(S);

    CHECK(tb.pending_count() == 1);
    CHECK(tb.is_pending(h));
    CHECK(!tb.is_stencil(h));
    CHECK(tb.stencil_count() == 0);

    // Second submission: promoted to stencil.
    tb.begin(S);
    body(long_body);
    tb.commit(S);

    CHECK(tb.is_stencil(h));
    CHECK(!tb.is_pending(h));
    CHECK(tb.stencil_count() == 1);
    CHECK(tb.stencil_hits(h) == 2);
    CHECK(tb.stencil_size(h) == long_body.size());

    // Third submission: hit count increments.
    tb.begin(S);
    body(long_body);
    tb.commit(S);

    CHECK(tb.stencil_hits(h) == 3);
    CHECK(tb.stencil_count() == 1);
  }

  SECTION("Different bodies get separate stencil entries") {
    // Two long bodies with different immediate values.
    auto body_a = LMR_of<false>(std::pair{M::MOD1_LO, u16(0xAAAA)}, std::pair{M::ID_HI, u16(0xBBBB)},
                                std::pair{M::DP_LO, u16(0xCCCC)});
    auto body_b = LMR_of<false>(std::pair{M::MOD1_LO, u16(0x1111)}, std::pair{M::ID_HI, u16(0x2222)},
                                std::pair{M::DP_LO, u16(0x3333)});

    auto h_a = tvm::TraceBuffer::hash({body_a.data(), body_a.size()});
    auto h_b = tvm::TraceBuffer::hash({body_b.data(), body_b.size()});

    // Sanity: different values produce different hashes.
    REQUIRE(h_a != h_b);

    // Submit body_a twice to promote it.
    tb.begin(S);
    body(body_a);
    tb.commit(S);
    tb.begin(S);
    body(body_a);
    tb.commit(S);

    CHECK(tb.is_stencil(h_a));
    CHECK(!tb.is_stencil(h_b));
    CHECK(tb.stencil_count() == 1);

    // Submit body_b twice to promote it.
    tb.begin(S);
    body(body_b);
    tb.commit(S);
    tb.begin(S);
    body(body_b);
    tb.commit(S);

    CHECK(tb.is_stencil(h_a));
    CHECK(tb.is_stencil(h_b));
    CHECK(tb.stencil_count() == 2);

    // Each stencil tracks hits independently.
    CHECK(tb.stencil_hits(h_a) == 2);
    CHECK(tb.stencil_hits(h_b) == 2);

    // Additional hit to body_a doesn't affect body_b.
    tb.begin(S);
    body(body_a);
    tb.commit(S);

    CHECK(tb.stencil_hits(h_a) == 3);
    CHECK(tb.stencil_hits(h_b) == 2);
  }

  SECTION("Promoted stencil executes correctly via STCALLHALT/RET") {
    // Use non-MOD registers so values survive the CALL-to-RET CLRMOD clearing.
    auto long_body = LMR_of<false>(std::pair{M::DP_LO, u16(0xAAAA)}, std::pair{M::ID_HI, u16(0xBBBB)},
                                   std::pair{M::OFF_LO, u16(0xCCCC)});
    auto h = tvm::TraceBuffer::hash({long_body.data(), long_body.size()});

    // 1st is pending, 2nd is promote.
    tb.begin(S);
    body(long_body);
    tb.commit(S);
    tb.begin(S);
    body(long_body);
    tb.commit(S);
    REQUIRE(tb.is_stencil(h));

    // 3rd submission uses STCALLHALT into the promoted stencil, which needs the trace buffer to resolve.
    tb.begin(S);
    body(long_body);
    auto loc = tb.commit(S);

    tvm::Interpreter blaster(mgr, std::make_unique<tvm::TraceApplyBackend>(mgr, nullptr, &tb));
    blaster.run(loc);
    CHECK(blaster.stopped());
    CHECK(blaster.regs().DP.lo == 0xAAAA);
    CHECK(blaster.regs().ID.hi == 0xBBBB);
    CHECK(blaster.regs().OFF.lo == 0xCCCC);
  }

  SECTION("CALLHALT returns to the buffer's HALT rather than falling through") {
    auto long_body = LMR_of<false>(std::pair{M::DP_LO, u16(0xAAAA)}, std::pair{M::ID_HI, u16(0xBBBB)},
                                   std::pair{M::OFF_LO, u16(0xCCCC)});
    for (int i = 0; i < 2; ++i) {
      tb.begin(S);
      body(long_body);
      tb.commit(S);
    }

    const auto at = tb.stencil_location(0); // Halt always pre-populated at index 0.
    const tvm::SegmentPair stencil{.hi = at.id.value, .lo = at.offset};

    // A call that returned to the next instruction would run this postfix.
    tb.begin(S);
    auto call = CallHalt{stencil}.encode();
    tb.emit_prefix(S, {call.data(), call.size()});
    auto marker = LMR_of<false>(std::pair{M::ACCESS, u16(0xBEEF)});
    tb.emit_postfix(S, {marker.data(), marker.size()});
    auto loc = tb.commit(S);

    tvm::Interpreter traced(mgr, std::make_unique<tvm::TraceApplyBackend>(mgr, nullptr, &tb));
    traced.run(loc);
    CHECK(traced.stop_cause() == tvm::StopCause::None);
    CHECK(traced.regs().DP.lo == 0xAAAA);
    CHECK(traced.regs().ACCESS == 0);

    // Without a trace buffer there is no HALT to return to.
    tvm::Interpreter plain(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    plain.run(loc);
    CHECK(plain.stop_cause() == tvm::StopCause::Unimplemented);
  }

  SECTION("STCALL returns to the next instruction, and refuses an unknown index") {
    auto long_body = LMR_of<false>(std::pair{M::DP_LO, u16(0xAAAA)}, std::pair{M::ID_HI, u16(0xBBBB)},
                                   std::pair{M::OFF_LO, u16(0xCCCC)});
    for (int i = 0; i < 2; ++i) {
      tb.begin(S);
      body(long_body);
      tb.commit(S);
    }
    REQUIRE(tb.stencil_count() == 1);

    auto program = [&](u16 index) {
      tb.begin(S);
      auto call = STCALL{index}.encode();
      tb.emit_prefix(S, {call.data(), call.size()});
      auto marker = LMR_of<false>(std::pair{M::ACCESS, u16(0xBEEF)});
      tb.emit_postfix(S, {marker.data(), marker.size()});
      return tb.commit(S);
    };

    tvm::Interpreter b(mgr, std::make_unique<tvm::TraceApplyBackend>(mgr, nullptr, &tb));
    b.run(program(0));
    CHECK(b.stop_cause() == tvm::StopCause::None);
    CHECK(b.regs().DP.lo == 0xAAAA);
    CHECK(b.regs().ACCESS == 0xBEEF); // came back for the postfix

    b.run(program(1));
    CHECK(b.stop_cause() == tvm::StopCause::StencilUnknown);
  }

  SECTION("Postfix is per-submission and not included in the stencil") {
    auto postfix = [&](auto enc) { tb.emit_postfix(S, {enc.data(), enc.size()}); };
    // Body uses non-MOD registers; postfix uses ACCESS (also non-MOD).
    auto long_body = LMR_of<false>(std::pair{M::DP_LO, u16(0xAAAA)}, std::pair{M::ID_HI, u16(0xBBBB)},
                                   std::pair{M::OFF_LO, u16(0xCCCC)});
    auto postfix_enc = LMR_of<false>(std::pair{M::ACCESS, u16(0xBEEF)});
    auto h = tvm::TraceBuffer::hash({long_body.data(), long_body.size()});

    // Submissions 1 & 2: body + custom postfix that sets ACCESS. Promotes on 2nd.
    tb.begin(S);
    body(long_body);
    postfix(postfix_enc);
    auto loc1 = tb.commit(S);
    tb.begin(S);
    body(long_body);
    postfix(postfix_enc);
    tb.commit(S);
    REQUIRE(tb.is_stencil(h));

    // Submission 3: same body, NO custom postfix, so just a STCALLHALT.
    // The ACCESS-setting instruction is deliberately dropped.
    tb.begin(S);
    body(long_body);
    auto loc3 = tb.commit(S);

    // Execute submission 1 (inlined body + custom postfix).
    {
      tvm::Interpreter b(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
      b.run(loc1);
      CHECK(b.stopped());
      CHECK(b.regs().DP.lo == 0xAAAA);
      CHECK(b.regs().ACCESS == 0xBEEF); // postfix executed
    }

    // Execute submission 3 (STCALLHALT, postfix dropped), which needs the trace buffer to resolve.
    {
      tvm::Interpreter b(mgr, std::make_unique<tvm::TraceApplyBackend>(mgr, nullptr, &tb));
      b.run(loc3);
      CHECK(b.stopped());
      CHECK(b.regs().DP.lo == 0xAAAA);  // body via stencil
      CHECK(b.regs().ACCESS == 0);       // postfix was dropped
    }
  }
}

TEST_CASE("tvm::Interpreter:  Stencil chain fills to a buffer boundary",
          "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  auto mgr = std::make_shared<pepp::bts::BufferManager>();
  tvm::TraceBuffer tb(mgr);
  // Stop one short: the point of interest is the state the *next* promotion would find.
  auto probe = promote_to_boundary(*mgr, tb, BOUNDARY_PROMOTIONS - 1);

  // The sizing arithmetic is only meaningful if the bodies actually promoted...
  CHECK(tb.stencil_count() == BOUNDARY_PROMOTIONS - 1);
  CHECK(tb.is_stencil(probe.hash));
  CHECK(tb.stencil_size(probe.hash) == BOUNDARY_BODY);

  // ...and if the remaining space fits another body but not another body + its 2-byte RET. That is the boundary the
  // next test drives into.
  auto *tbuf = mgr->find(probe.id);
  REQUIRE(tbuf != nullptr);
  size_t remaining = pepp::bts::Buffer::SIZE - tbuf->used_capacity();
  CHECK(remaining >= BOUNDARY_BODY);
  CHECK(remaining < BOUNDARY_BODY + 2);
}

// A body sized flush against a buffer boundary is the case where the body and its trailing RET could end up in
// different buffers, since a chain append that does not fit rolls over to a fresh buffer.
TEST_CASE("tvm::Interpreter:  A promoted stencil keeps its RET in the same buffer",
          "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  auto mgr = std::make_shared<pepp::bts::BufferManager>();
  tvm::TraceBuffer tb(mgr);
  auto probe = promote_to_boundary(*mgr, tb);

  auto *tbuf = mgr->find(probe.id);
  REQUIRE(tbuf != nullptr);

  // The RET is reached by falling out of the body, so it has to sit immediately after it in the same buffer.
  // Otherwise the stencil runs off the end into whatever follows and never returns to the caller's postfix.
  const size_t ret_at = (size_t)probe.offset + BOUNDARY_BODY;
  const bool ret_in_buffer = ret_at + 2 <= tbuf->span().size();
  CHECK(ret_in_buffer);
  CHECK(tbuf->used_capacity() >= ret_at + 2);

  // Guarded rather than REQUIREd, since reading those bytes when they are out of range would run off the buffer.
  if (ret_in_buffer) {
    constexpr auto ret = tvm::EncodedOp::Ret<0>{}.encode();
    CHECK(tbuf->data()[ret_at + 0] == ret[0]);
    CHECK(tbuf->data()[ret_at + 1] == ret[1]);
  }
}
