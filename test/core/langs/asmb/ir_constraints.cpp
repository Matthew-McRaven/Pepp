/*
 * Copyright (c) 2026. Stanley Warford, Matthew McRaven
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "core/langs/asmb/ir_constraints.hpp"
#include <catch.hpp>

namespace {
using pepp::tc::ArgumentConstraint;
using pepp::tc::expr::Value;
using enum pepp::tc::expr::Signedness;
Value unsigned_value(u64 x) { return {x, {32, Bits}}; }
Value signed_value(i64 x) { return {static_cast<u64>(x) & 0xFFFF'FFFF, {32, Signed}}; }
} // namespace

TEST_CASE("Argument constraints", "[scope:core][scope:core.langs][kind:unit][arch:*]") {
  using pepp::tc::satisfies;
  SECTION("A Bits constraint accepts either signed or unsigned") {
    const ArgumentConstraint byte{8, Bits};
    CHECK(satisfies(signed_value(-128), byte));
    CHECK(satisfies(signed_value(-1), byte));
    CHECK(satisfies(unsigned_value(255), byte));
    CHECK_FALSE(satisfies(signed_value(-129), byte));
    CHECK_FALSE(satisfies(unsigned_value(256), byte));
    CHECK_FALSE(satisfies(unsigned_value(0xFFFF'FFFF), byte));
  }
  SECTION("Signed and unsigned constraints") {
    const ArgumentConstraint imm{12, Signed}, shamt{5, Unsigned};
    CHECK(satisfies(unsigned_value(0x7FF), imm));
    CHECK(satisfies(signed_value(-2048), imm));
    CHECK_FALSE(satisfies(unsigned_value(0x800), imm));
    CHECK_FALSE(satisfies(signed_value(-2049), imm));
    CHECK(satisfies(unsigned_value(31), shamt));
    CHECK_FALSE(satisfies(unsigned_value(32), shamt));
    CHECK_FALSE(satisfies(signed_value(-1), shamt));
  }
  SECTION("Aligned constraints") {
    const ArgumentConstraint branch{13, Signed, 2};
    CHECK(satisfies(unsigned_value(4094), branch));
    CHECK(satisfies(signed_value(-4096), branch));
    CHECK_FALSE(satisfies(unsigned_value(4096), branch));
    CHECK_FALSE(satisfies(unsigned_value(3), branch));
    CHECK_FALSE(pepp::tc::aligned(signed_value(-3), branch));
  }
}
