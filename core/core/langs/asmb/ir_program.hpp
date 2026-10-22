/*
 * Copyright (c) 2025-2026 J. Stanley Warford, Matthew McRaven
 *
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

#pragma once
#include "core/compile/ir_linear/line_base.hpp"
#include "flat/flat_map.hpp"

namespace pepp::tc {
using IRProgram = std::vector<std::shared_ptr<tc::LinearIR>>;

// Assigning addresses should be O(1) and incur O(1) memory allocations.
// A pre-sized vector (essentially an arena allocator) is a natural container.
// A flat_map is a container adaptor which presents a map api over a different ordered container, but you still get a
// convenient O(lgn) find without lots of evil stl magic.
template <typename Address> using IRMemoryAddressPair = std::pair<const LinearIR *, Address>;
namespace detail {
// Only compare based on the address of ir::LinearIR *, ignoring ir::attr::Address.
// We already have a guarentee that all IR are unique.
template <typename Address> struct IRComparator {
  bool operator()(const IRMemoryAddressPair<Address> &lhs, const IRMemoryAddressPair<Address> &rhs) const {
    return lhs.first < rhs.first;
  }
  bool operator()(LinearIR *const lhs, LinearIR *const &rhs) const { return lhs < rhs; }
  bool operator()(const LinearIR *const lhs, const LinearIR *const &rhs) const { return lhs < rhs; }
};
} // namespace detail
template <typename Address>
using IRMemoryAddressTable = fc::flat_map<std::vector<IRMemoryAddressPair<Address>>, detail::IRComparator<Address>>;
} // namespace pepp::tc
