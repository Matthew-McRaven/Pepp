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
#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>
#include <variant>
#include <vector>
#include "core/compile/source/location.hpp"
#include "core/compile/source/seekable.hpp"
#include "core/langs/expr/error.hpp"
#include "core/langs/expr/ir.hpp"

/*
 * Precedence climbing over a subset of C's binary operators, all assuming left-associativity.
 *   <expression> -> <unary> (BINARY_OP <expression of higher precedence>)*
 *   <unary>      -> UNARY_OP <unary> | <primary>
 *   <primary>    -> INTEGER | CHARACTER | IDENTIFIER | ( <expression> )
 */
namespace pepp::tc::expr {

// The text did not start with an expression and nothing was consumed.
struct NoExpression {};

// The longest valid match up to `after`.
struct Parsed {
  Tree tree;
  // Source span of each node, indexed by NodeId. An operator's span covers its whole subexpression, excluding parens.
  std::vector<support::LocationInterval> locations;
  size_t length = 0;
  support::SeekableData after;
};

using ParseResult = std::variant<NoExpression, Parsed, Error>;

using IdentifierPool = std::unordered_set<std::string>;

// Parse the longest expression starting at cursor. Parsing stops at the first token which cannot continue the
// expression, such as a comma or a newline. If this is being called from inside another parser, the caller can advance
// their own cursor by taking Parsed::after. The identifier pool should be shared with the caller to reduce temporary
// allocations when being used as a sub-parser.
ParseResult parse(support::SeekableData cursor, std::shared_ptr<IdentifierPool> pool);
// As above, for an expression held in a string (e.g., typed into the debugger).
ParseResult parse(std::string_view text, support::Location origin = support::Location(0, 0),
                  std::shared_ptr<IdentifierPool> pool = nullptr);

} // namespace pepp::tc::expr
