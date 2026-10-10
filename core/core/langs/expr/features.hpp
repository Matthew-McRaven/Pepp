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

namespace pepp::tc::expr {

// Grammar which varies between the languages sharing this parser.
struct Features {
  // How to interpret the full-stop character?
  enum class Dot : u8 {
    Forbidden,  // Not part of the grammar.
    Identifier, // The location counter. TODO: also allowed in identifiers, e.g., .L1 (a GNU local symbol).
    Operator,   // Member access (e.g., the debugger's a.b). Not implemented yet.
  } dot = Dot::Forbidden;
};

} // namespace pepp::tc::expr
