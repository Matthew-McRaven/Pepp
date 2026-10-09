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
#pragma once
#include "core/integers.h"
#include "core/math/bitmanip/mask.hpp"

namespace pepp::tc::expr {

// Bits have no signedness but still have a width. Signed + unsigned provide interpretations for those bits.
// Many operations can be performed without knowing the signedness as long as the sizes match.
enum class Signedness : u8 { Bits, Signed, Unsigned };
struct Type {
  u8 bits = 0; // 8, 16, 32, or 64.
  Signedness sign = Signedness::Bits;
  bool operator==(const Type &) const = default;
};

// Storage for the largest size bit pattern + type info.
struct Value {
  // Bits are always truncated to the width of type.
  u64 bits = 0;
  Type type;
  bool operator==(const Value &) const = default;
  // The bits as a two's complement number of type's width.
  constexpr i64 as_signed() const { return static_cast<i64>(::bits::sign_extend(bits, type.bits / 8)); }
};

} // namespace pepp::tc::expr
