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
#include <optional>
#include <string>
#include "core/integers.h"
#include "core/langs/expr/value.hpp"

namespace pepp::tc {

// Restrictions that
struct ArgumentConstraint {
  u8 bits = 0;
  // Bits allows signed or unsigned values.
  expr::Signedness sign = expr::Signedness::Bits;
  // Values must be a multiple of this to satisfy the constraint
  u8 align = 1;
  bool operator==(const ArgumentConstraint &) const = default;
};

// Checks that the value is in range and meets the alignment requirement.
bool satisfies(expr::Value value, ArgumentConstraint constraint);
// True if value is a multiple of constraint.align.
bool aligned(expr::Value value, ArgumentConstraint constraint);

// e.g. "signed 12 bits" or "8 bits".
std::string to_string(ArgumentConstraint constraint);

} // namespace pepp::tc
