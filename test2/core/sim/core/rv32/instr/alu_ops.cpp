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
#include <random>
#include "core/sim/cores/cpu/rv32/rv_i_ops.hpp"

namespace {
// Golden references written in a
constexpr u32 ref_add(u32 a, u32 b) { return u32((u64(a) + u64(b))); }
constexpr u32 ref_sub(u32 a, u32 b) { return u32((u64(a) - u64(b))); }
constexpr u32 ref_sll(u32 a, u32 b) { return u32((u64(a) << (b & 0x1F))); }
constexpr u32 ref_srl(u32 a, u32 b) { return u32(u64(a) >> (b & 0x1F)); }
constexpr u32 ref_sltu(u32 a, u32 b) { return u32(u64(a) < u64(b)); }
constexpr u32 ref_xor(u32 a, u32 b) { return a ^ b; }
constexpr u32 ref_or(u32 a, u32 b) { return a | b; }
constexpr u32 ref_and(u32 a, u32 b) { return a & b; }
constexpr u32 ref_slt(u32 a, u32 b) {
  const bool neg_a = a & 0x80000000u, neg_b = b & 0x80000000u;
  return neg_a != neg_b ? u32(neg_a) : u32(a < b);
}
constexpr u32 ref_sra(u32 a, u32 b) {
  // Extract 5-bit shift amount with an early return if 0.
  const u32 shamt = b & 0x1F;
  if (shamt == 0) return a;
  // Implement arithmetic shift as a logic shift...
  u32 out = a >> shamt;
  // .. followed sign-extension for the places which were shifted out of.
  if (a & 0x80000000u) out |= ~0u << (32 - shamt);
  return out;
}

using Fn = u32 (*)(u32, u32);
struct Op {
  const char *name;
  Fn impl, ref;
};
constexpr std::array<Op, 10> OPS{{
    {"add", rv32::op_add, ref_add},   {"sub", rv32::op_sub, ref_sub},
    {"sll", rv32::op_sll, ref_sll},   {"slt", rv32::op_slt, ref_slt},
    {"sltu", rv32::op_sltu, ref_sltu}, {"xor", rv32::op_xor, ref_xor},
    {"srl", rv32::op_srl, ref_srl},   {"sra", rv32::op_sra, ref_sra},
    {"or", rv32::op_or, ref_or},      {"and", rv32::op_and, ref_and},
}};

// Sign boundaries, shift-amount boundaries, and a couple of alternating patterns.
constexpr std::array<u32, 16> BOUNDS{0u,          1u,          2u,          0x1Fu,
                                     0x20u,       0x21u,       0x0000FFFFu, 0xFFFF0000u,
                                     0x55555555u, 0xAAAAAAAAu, 0x7FFFFFFFu, 0x80000000u,
                                     0x80000001u, 0xFFFFFFFEu, 0xFFFFFFFFu, 0x00000100u};
} // namespace

// Since ALU operations are implemented with pure functions, we can validate them against a reference implementation
// without needing to create a full system.Both register-register and register-immediate variants delegate to these
// functions.
TEST_CASE("RV32I ALU operations", "[scope:core][scope:core.sim][kind:unit][arch:rv]") {
  SECTION("every boundary pair matches an independent reference") {
    for (const auto &op : OPS)
      for (u32 a : BOUNDS)
        for (u32 b : BOUNDS) {
          CAPTURE(op.name, a, b);
          CHECK(op.impl(a, b) == op.ref(a, b));
        }
  }

  SECTION("pseudo-random values") {
    std::mt19937 rng(0xC0FFEEu);
    std::uniform_int_distribution<u32> any(0, 0xFFFFFFFFu);
    for (int i = 0; i < 4096; ++i) {
      const u32 a = any(rng), b = any(rng);
      for (const auto &op : OPS) {
        CAPTURE(op.name, a, b, i);
        CHECK(op.impl(a, b) == op.ref(a, b));
      }
    }
  }

  SECTION("shifts consume only rs2[4:0]") {
    // Masking means a shift of 32 is a shift of zero, which no amount of agreement between two
    // implementations that both mask would reveal.
    for (u32 v : BOUNDS) {
      CAPTURE(v);
      CHECK(rv32::op_sll(v, 32) == v);
      CHECK(rv32::op_srl(v, 32) == v);
      CHECK(rv32::op_sra(v, 32) == v);
      CHECK(rv32::op_sll(v, 33) == rv32::op_sll(v, 1));
    }
  }

  SECTION("signed and unsigned comparisons differ") {
    CHECK(rv32::op_slt(0xFFFFFFFFu, 0u) == 1);  // -1 < 0
    CHECK(rv32::op_sltu(0xFFFFFFFFu, 0u) == 0); // 4294967295 > 0
    CHECK(rv32::op_slt(0x80000000u, 0x7FFFFFFFu) == 1);
    CHECK(rv32::op_sltu(0x80000000u, 0x7FFFFFFFu) == 0);
    CHECK(rv32::op_sra(0x80000000u, 31) == 0xFFFFFFFFu);
    CHECK(rv32::op_srl(0x80000000u, 31) == 0x00000001u);
  }
}

// Validate most important properties at compile time, since ops are constexpr.
static_assert(rv32::op_add(0xFFFFFFFFu, 1u) == 0u, "add discards overflow");
static_assert(rv32::op_slt(0xFFFFFFFFu, 0u) == 1u, "slt is signed");
static_assert(rv32::op_sltu(0xFFFFFFFFu, 0u) == 0u, "sltu is unsigned");
static_assert(rv32::op_sra(0x80000000u, 31u) == 0xFFFFFFFFu, "sra propagates the sign");
static_assert(rv32::op_srl(0x80000000u, 31u) == 1u, "srl does not");
static_assert(rv32::op_sll(1u, 32u) == 1u, "shift amount is masked to five bits");
