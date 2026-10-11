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
#include "core/macros.hpp"
#include "fmt/format.h"

namespace {
using pepp::tc::expr::Signedness;
} // namespace

bool pepp::tc::satisfies(expr::Value value, ArgumentConstraint constraint) {
  const u8 n = constraint.bits;
  if (n == 0) return false;
  else if (value.type.sign == Signedness::Signed && value.as_signed() < 0) {
    const i64 x = value.as_signed();
    if (constraint.sign == Signedness::Unsigned) return false;
    else if (n < 64 && x < -(i64(1) << (n - 1))) return false;
    return aligned(value, constraint);
  }
  const u64 x = value.type.sign == Signedness::Signed ? static_cast<u64>(value.as_signed()) : value.bits;
  // A Signed constraint holds one bit less of magnitude than the others.
  const u8 magnitude = constraint.sign == Signedness::Signed ? n - 1 : n;
  if (magnitude < 64 && x > (u64(1) << magnitude) - 1) return false;
  return aligned(value, constraint);
}

bool pepp::tc::aligned(expr::Value value, ArgumentConstraint constraint) {
  const u64 x = value.type.sign == Signedness::Signed ? static_cast<u64>(value.as_signed()) : value.bits;
  return constraint.align <= 1 || x % constraint.align == 0;
}

std::string pepp::tc::to_string(ArgumentConstraint constraint) {
  switch (constraint.sign) {
  case Signedness::Signed: return fmt::format("signed {} bits", constraint.bits);
  case Signedness::Unsigned: return fmt::format("unsigned {} bits", constraint.bits);
  case Signedness::Bits: return fmt::format("{} bits", constraint.bits);
  }
  PEPP_UNREACHABLE();
}
