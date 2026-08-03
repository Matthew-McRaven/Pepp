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

// Sized so that after two promotions the template chain's buffer has exactly BOUNDARY_BODY bytes left: each promotion
// consumes BODY + 2 (the trailing RET), and 2 * (BODY + 2) + BODY == 65536. A third body therefore fits on its own but
// not alongside its RET, which is the only situation where the two could be separated.
constexpr size_t BOUNDARY_BODY = 21844;
constexpr size_t BOUNDARY_PROMOTIONS = 3;

struct TemplateProbe {
  u32 hash = 0;
  // Where the CALL emitted by the last promoted submission points.
  pepp::bts::Buffer::ID id{};
  u16 offset = 0;
};

// Promote `promotions` distinct bodies by submitting each twice, then read the template location back out of the CALL
// that replaced the last body.
TemplateProbe promote_to_boundary(pepp::bts::BufferManager &mgr, tvm::TraceBuffer &tb,
                                  size_t promotions = BOUNDARY_PROMOTIONS) {
  constexpr Device::ID S{1};
  TemplateProbe probe;
  pepp::bts::Buffer::Location loc{};

  for (size_t i = 0; i < promotions; ++i) {
    std::vector<u8> body(BOUNDARY_BODY, static_cast<u8>(0xA0 + i));
    probe.hash = tvm::TraceBuffer::hash({body.data(), body.size()});
    for (int pass = 0; pass < 2; ++pass) {
      tb.begin(S);
      tb.emit_body(S, {body.data(), body.size()});
      loc = tb.commit(S);
    }
  }

  // With no prefix emitted, the promoted submission's program is [CALL][HALT], so the CALL is at the start.
  auto *code = mgr.find(loc.id);
  REQUIRE(code != nullptr);
  const auto *p = code->data() + loc.offset;
  REQUIRE(p[0] == 2);                              // word_len
  REQUIRE(p[1] == (0x40 | (u8)tvm::Opcode::CALL)); // clrmod | opcode
  probe.offset = (u16)p[2] | ((u16)p[3] << 8);     // next_ip.lo
  probe.id = pepp::bts::Buffer::ID{(u16)((u16)p[4] | ((u16)p[5] << 8))}; // next_ip.hi
  return probe;
}

} // namespace

TEST_CASE("tvm::Interpreter:  Template promotion", "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
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

    CHECK(tb.template_count() == 0);
    CHECK(tb.pending_count() == 0);

    // First submission: body enters pending set.
    tb.begin(S);
    body(short_body);
    tb.commit(S);

    CHECK(tb.pending_count() == 1);
    CHECK(tb.is_pending(h));
    CHECK(!tb.is_template(h));
    CHECK(tb.template_count() == 0);

    // Second submission: body is too short to promote; stays not-promoted.
    tb.begin(S);
    body(short_body);
    tb.commit(S);

    CHECK(!tb.is_template(h));
    CHECK(tb.template_count() == 0);

    // Several more submissions: still never promoted.
    for (int i = 0; i < 5; ++i) {
      tb.begin(S);
      body(short_body);
      tb.commit(S);
    }

    CHECK(!tb.is_template(h));
    CHECK(tb.template_count() == 0);
  }

  SECTION("Long body is promoted on second occurrence") {
    // LMR_of with 3 register pairs encodes to 10 bytes (5 words), above the threshold.
    auto long_body = LMR_of<false>(std::pair{M::MOD1_LO, u16(0x1234)}, std::pair{M::ID_HI, u16(0xFEED)},
                                   std::pair{M::DP_LO, u16(0xBEEF)});
    auto h = tvm::TraceBuffer::hash({long_body.data(), long_body.size()});

    CHECK(tb.template_count() == 0);
    CHECK(tb.pending_count() == 0);

    // First submission: enters pending set.
    tb.begin(S);
    body(long_body);
    tb.commit(S);

    CHECK(tb.pending_count() == 1);
    CHECK(tb.is_pending(h));
    CHECK(!tb.is_template(h));
    CHECK(tb.template_count() == 0);

    // Second submission: promoted to template.
    tb.begin(S);
    body(long_body);
    tb.commit(S);

    CHECK(tb.is_template(h));
    CHECK(!tb.is_pending(h));
    CHECK(tb.template_count() == 1);
    CHECK(tb.template_hits(h) == 2);
    CHECK(tb.template_size(h) == long_body.size());

    // Third submission: hit count increments.
    tb.begin(S);
    body(long_body);
    tb.commit(S);

    CHECK(tb.template_hits(h) == 3);
    CHECK(tb.template_count() == 1);
  }

  SECTION("Different bodies get separate template entries") {
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

    CHECK(tb.is_template(h_a));
    CHECK(!tb.is_template(h_b));
    CHECK(tb.template_count() == 1);

    // Submit body_b twice to promote it.
    tb.begin(S);
    body(body_b);
    tb.commit(S);
    tb.begin(S);
    body(body_b);
    tb.commit(S);

    CHECK(tb.is_template(h_a));
    CHECK(tb.is_template(h_b));
    CHECK(tb.template_count() == 2);

    // Each template tracks hits independently.
    CHECK(tb.template_hits(h_a) == 2);
    CHECK(tb.template_hits(h_b) == 2);

    // Additional hit to body_a doesn't affect body_b.
    tb.begin(S);
    body(body_a);
    tb.commit(S);

    CHECK(tb.template_hits(h_a) == 3);
    CHECK(tb.template_hits(h_b) == 2);
  }

  SECTION("Promoted template executes correctly via CALL/RET") {
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
    REQUIRE(tb.is_template(h));

    // 3rd submission uses CALL into the promoted template.
    tb.begin(S);
    body(long_body);
    auto loc = tb.commit(S);

    tvm::Interpreter blaster(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
    blaster.run(loc);
    CHECK(blaster.stopped());
    CHECK(blaster.regs().DP.lo == 0xAAAA);
    CHECK(blaster.regs().ID.hi == 0xBBBB);
    CHECK(blaster.regs().OFF.lo == 0xCCCC);
  }

  SECTION("Postfix is per-submission, not baked into template") {
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
    REQUIRE(tb.is_template(h));

    // Submission 3: same body, NO custom postfix, CALL + HALT only.
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

    // Execute submission 3 (template CALL, postfix dropped).
    {
      tvm::Interpreter b(mgr, std::make_unique<tvm::ApplyBackend>(mgr));
      b.run(loc3);
      CHECK(b.stopped());
      CHECK(b.regs().DP.lo == 0xAAAA);  // body via template
      CHECK(b.regs().ACCESS == 0);       // postfix was dropped
    }
  }
}

TEST_CASE("tvm::Interpreter:  Template chain fills to a buffer boundary",
          "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  auto mgr = std::make_shared<pepp::bts::BufferManager>();
  tvm::TraceBuffer tb(mgr);
  // Stop one short: the point of interest is the state the *next* promotion would find.
  auto probe = promote_to_boundary(*mgr, tb, BOUNDARY_PROMOTIONS - 1);

  // The sizing arithmetic is only meaningful if the bodies actually promoted...
  CHECK(tb.template_count() == BOUNDARY_PROMOTIONS - 1);
  CHECK(tb.is_template(probe.hash));
  CHECK(tb.template_size(probe.hash) == BOUNDARY_BODY);

  // ...and if they leave exactly one body's worth of room behind. Another body fits; another body plus its RET does
  // not. That is the boundary the next test drives into, and it is a property of the fill alone -- where the third
  // promotion actually lands is what that test is about.
  auto *tbuf = mgr->find(probe.id);
  REQUIRE(tbuf != nullptr);
  CHECK(tbuf->used_capacity() + BOUNDARY_BODY == pepp::bts::Buffer::SIZE);
  CHECK(tbuf->used_capacity() + BOUNDARY_BODY + 2 > pepp::bts::Buffer::SIZE);
}

// A body sized flush against a buffer boundary is the case where the body and its trailing RET could end up in
// different buffers, since a chain append that does not fit rolls over to a fresh buffer.
TEST_CASE("tvm::Interpreter:  A promoted template keeps its RET in the same buffer",
          "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  auto mgr = std::make_shared<pepp::bts::BufferManager>();
  tvm::TraceBuffer tb(mgr);
  auto probe = promote_to_boundary(*mgr, tb);

  auto *tbuf = mgr->find(probe.id);
  REQUIRE(tbuf != nullptr);

  // The RET is reached by falling out of the body, so it has to sit immediately after it in the same buffer.
  // Otherwise the template runs off the end into whatever follows and never returns to the caller's postfix.
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
