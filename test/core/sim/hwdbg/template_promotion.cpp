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

#include "core/sim/debugger/register_blaster.hpp"
#include "core/sim/debugger/tvm_encoding.hpp"
#include "core/sim/debugger/tvm_tracebuffer.hpp"

TEST_CASE("Template promotion", "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  auto mgr = std::make_shared<pepp::bts::BufferManager>();
  using namespace tvm::EncodedOp;
  using M = tvm::RegMask;
  tvm::TraceBuffer tb(mgr, 1);
  constexpr u16 S = 0;

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
    tb.end(S);

    CHECK(tb.pending_count() == 1);
    CHECK(tb.is_pending(h));
    CHECK(!tb.is_template(h));
    CHECK(tb.template_count() == 0);

    // Second submission: body is too short to promote; stays not-promoted.
    tb.begin(S);
    body(short_body);
    tb.end(S);

    CHECK(!tb.is_template(h));
    CHECK(tb.template_count() == 0);

    // Several more submissions: still never promoted.
    for (int i = 0; i < 5; ++i) {
      tb.begin(S);
      body(short_body);
      tb.end(S);
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
    tb.end(S);

    CHECK(tb.pending_count() == 1);
    CHECK(tb.is_pending(h));
    CHECK(!tb.is_template(h));
    CHECK(tb.template_count() == 0);

    // Second submission: promoted to template.
    tb.begin(S);
    body(long_body);
    tb.end(S);

    CHECK(tb.is_template(h));
    CHECK(!tb.is_pending(h));
    CHECK(tb.template_count() == 1);
    CHECK(tb.template_hits(h) == 2);
    CHECK(tb.template_size(h) == long_body.size());

    // Third submission: hit count increments.
    tb.begin(S);
    body(long_body);
    tb.end(S);

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
    tb.end(S);
    tb.begin(S);
    body(body_a);
    tb.end(S);

    CHECK(tb.is_template(h_a));
    CHECK(!tb.is_template(h_b));
    CHECK(tb.template_count() == 1);

    // Submit body_b twice to promote it.
    tb.begin(S);
    body(body_b);
    tb.end(S);
    tb.begin(S);
    body(body_b);
    tb.end(S);

    CHECK(tb.is_template(h_a));
    CHECK(tb.is_template(h_b));
    CHECK(tb.template_count() == 2);

    // Each template tracks hits independently.
    CHECK(tb.template_hits(h_a) == 2);
    CHECK(tb.template_hits(h_b) == 2);

    // Additional hit to body_a doesn't affect body_b.
    tb.begin(S);
    body(body_a);
    tb.end(S);

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
    tb.end(S);
    tb.begin(S);
    body(long_body);
    tb.end(S);
    REQUIRE(tb.is_template(h));

    // 3rd submission uses CALL into the promoted template.
    tb.begin(S);
    body(long_body);
    auto loc = tb.end(S);

    RegisterBlaster blaster(mgr);
    blaster.run_direct(loc);
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
    auto loc1 = tb.end(S);
    tb.begin(S);
    body(long_body);
    postfix(postfix_enc);
    tb.end(S);
    REQUIRE(tb.is_template(h));

    // Submission 3: same body, NO custom postfix, CALL + HALT only.
    // The ACCESS-setting instruction is deliberately dropped.
    tb.begin(S);
    body(long_body);
    auto loc3 = tb.end(S);

    // Execute submission 1 (inlined body + custom postfix).
    {
      RegisterBlaster b(mgr);
      b.run_direct(loc1);
      CHECK(b.stopped());
      CHECK(b.regs().DP.lo == 0xAAAA);
      CHECK(b.regs().ACCESS == 0xBEEF); // postfix executed
    }

    // Execute submission 3 (template CALL, postfix dropped).
    {
      RegisterBlaster b(mgr);
      b.run_direct(loc3);
      CHECK(b.stopped());
      CHECK(b.regs().DP.lo == 0xAAAA);  // body via template
      CHECK(b.regs().ACCESS == 0);       // postfix was dropped
    }
  }
}
