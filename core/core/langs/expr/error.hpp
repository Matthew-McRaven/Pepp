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
#include "core/compile/source/location.hpp"
#include "core/langs/expr/ir.hpp"

namespace pepp::tc::expr {

// A problem found while parsing an expression and the location in the lexer's buffer where it occured.
struct Error {
  support::LocationInterval location;
  std::string message;
};

// An operation which failed during evaluation. Callers should map the node to a source location.
struct EvaluationError {
  std::optional<NodeId> node; // nullopt if the tree is empty.
  std::string message;
};

} // namespace pepp::tc::expr
