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

#include <catch.hpp>
#include <array>
#include <memory>
#include "core/compile/ir_value/expr.hpp"
#include "core/compile/symbol/entry.hpp"
#include "core/compile/symbol/value.hpp"

namespace {
using namespace pepp::ast;
using Op = InfixExpression::Op;
using Expr = InfixExpression;

std::shared_ptr<IRValue> dec(u64 value, u8 size) { return std::make_shared<UnsignedDecimal>(value, size); }
std::shared_ptr<IRValue> hex(u64 value, u8 size) { return std::make_shared<Hexadecimal>(value, size); }
std::shared_ptr<IRValue> add(std::shared_ptr<IRValue> lhs, std::shared_ptr<IRValue> rhs, u8 size) {
  return std::make_shared<Expr>(Op::Addition, std::move(lhs), std::move(rhs), size);
}
// Constant-valued symbol; callers may reassign entry->value later.
std::shared_ptr<pepp::core::symbol::Entry> constant_symbol(std::string_view name, u64 value, u8 size) {
  auto entry = std::make_shared<pepp::core::symbol::Entry>(name);
  entry->value = std::make_shared<pepp::core::symbol::ConstantValue>(
      bits::MaskedBits{.byteCount = size, .bitPattern = value, .mask = (1ULL << (8 * size)) - 1});
  return entry;
}
} // namespace

TEST_CASE("pepp infix expressions", "[scope:core][scope:core.compile][kind:unit][arch:*]") {
  SECTION("Nil and missing operands evaluate to 0") {
    CHECK(Expr().value_as<u32>() == 0);
    CHECK(Expr().string() == "0");
    CHECK(Expr(Op::Nil, dec(5, 1), dec(6, 1), 1).value_as<u8>() == 0);
    CHECK(Expr(Op::Addition, dec(5, 1), nullptr, 1).value_as<u8>() == 0);
    CHECK(Expr(Op::Addition, nullptr, dec(5, 1), 1).value_as<u8>() == 0);
  }
  SECTION("Addition") {
    auto sum = Expr(Op::Addition, dec(3, 1), dec(4, 1), 1);
    CHECK(sum.serialized_size() == 1);
    CHECK(sum.value_as<u8>() == 7);
    CHECK(sum.string() == "3 + 4");
    CHECK(sum.raw_string() == "3 + 4");
    CHECK(Expr(Op::Addition, dec(300, 2), dec(400, 2), 2).value_as<u16>() == 700);
    CHECK(Expr(Op::Addition, dec(0x10000, 4), dec(0x20000, 4), 4).value_as<u32>() == 0x30000);
  }
  SECTION("Result is truncated to size") {
    CHECK(Expr(Op::Addition, dec(0xFF, 1), dec(1, 1), 1).value_as<u16>() == 0);
    CHECK(Expr(Op::Addition, dec(0xFF, 1), dec(0xFF, 1), 1).value_as<u16>() == 0xFE);
    CHECK(Expr(Op::Addition, dec(0x7FFF, 2), dec(1, 2), 2).value_as<u32>() == 0x8000);
    CHECK(Expr(Op::Addition, dec(0xFFFF, 2), dec(1, 2), 2).value_as<u32>() == 0);
    // Operands wider than the result are still added in full before truncating.
    CHECK(Expr(Op::Addition, dec(0x100, 2), dec(0x2FF, 2), 1).value_as<u16>() == 0xFF);
  }
  SECTION("Operands may be symbols") {
    auto sym = constant_symbol("sym", 0x10, 2);
    auto sum = Expr(Op::Addition, hex(0xFE, 1), std::make_shared<Symbolic>(2, sym), 2);
    CHECK(sum.value_as<u16>() == 0x10E);
    CHECK(sum.string() == "0xFE + sym");
  }
  SECTION("Arguments are read when evaluated, not during construction") {
    auto sym = constant_symbol("sym", 1, 2);
    auto sum = Expr(Op::Addition, dec(10, 2), std::make_shared<Symbolic>(2, sym), 2);
    CHECK(sum.value_as<u16>() == 11);
    sym->value = std::make_shared<pepp::core::symbol::ConstantValue>(
        bits::MaskedBits{.byteCount = 2, .bitPattern = 90, .mask = 0xFFFF});
    CHECK(sum.value_as<u16>() == 100);
  }
  SECTION("Serializes in the requested byte order") {
    auto sum = Expr(Op::Addition, hex(0x1200, 2), hex(0x34, 2), 2);
    std::array<u8, 2> out{};
    (void)sum.serialize(out, bits::Order::BigEndian);
    CHECK(out == std::array<u8, 2>{0x12, 0x34});
    (void)sum.serialize(out, bits::Order::LittleEndian);
    CHECK(out == std::array<u8, 2>{0x34, 0x12});
  }
}

TEST_CASE("pepp nested infix expressions", "[scope:core][scope:core.compile][kind:unit][arch:*]") {
  SECTION("Nested on either side") {
    auto left = Expr(Op::Addition, add(dec(1, 1), dec(2, 1), 1), dec(4, 1), 1);
    auto right = Expr(Op::Addition, dec(1, 1), add(dec(2, 1), dec(4, 1), 1), 1);
    auto both = Expr(Op::Addition, add(dec(1, 1), dec(2, 1), 1), add(dec(4, 1), dec(8, 1), 1), 1);
    CHECK(left.value_as<u8>() == 7);
    CHECK(right.value_as<u8>() == 7);
    CHECK(both.value_as<u8>() == 15);
    CHECK(left.string() == "1 + 2 + 4");
    CHECK(right.string() == "1 + 2 + 4");
    CHECK(both.string() == "1 + 2 + 4 + 8");
  }
  SECTION("Each level respects declared size") {
    // Inner byte-sized sum wraps to 0 before being added to a wider operand.
    auto wrapped = Expr(Op::Addition, add(dec(0xFF, 1), dec(1, 1), 1), dec(5, 2), 2);
    CHECK(wrapped.value_as<u16>() == 5);
    // A wider inner sum keeps its carry, which the narrow outer then truncates.
    auto widened = Expr(Op::Addition, add(dec(0xFF, 1), dec(1, 1), 2), dec(5, 1), 1);
    CHECK(widened.value_as<u16>() == 5);
    auto kept = Expr(Op::Addition, add(dec(0xFF, 1), dec(1, 1), 2), dec(5, 2), 2);
    CHECK(kept.value_as<u16>() == 0x105);
  }
}
