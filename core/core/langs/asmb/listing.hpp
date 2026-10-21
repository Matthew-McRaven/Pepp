/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
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
#include <algorithm>
#include <functional>
#include <string>
#include <vector>
#include "core/compile/ir_linear/attr_comment.hpp"
#include "core/integers.h"
#include "core/langs/asmb/elf_symtab.hpp"
#include "core/langs/asmb/ir_program.hpp"

namespace pepp::tc {

// A listing's rows before addresses and object code are filled in, which come from the ELF.
struct ListingLayout {
  // Source-column text of each printed row. Continuation rows are empty.
  std::vector<std::string> text;
  // Each listed line's first row, 0-based. Numbers the rows of .debug.pepp.listing_line.
  IR2ListingLineMap rows;
};

// Formats one line's source column.
using ListingFormatter = std::function<std::string(const LinearIR &)>;

// Lays out the listing of `source_order`, with `bytes_per_line` bytes of object code per row. Lines hidden from the
// listing get no row, and a line whose object code is suppressed gets one.
template <typename Address>
ListingLayout layout_listing(const IRProgram &source_order, const IRMemoryAddressTable<Address> &addresses,
                             u32 bytes_per_line, const ListingFormatter &format) {
  ListingLayout ret;
  for (const auto &line : source_order) {
    if (hidden_from_listing(*line)) continue;
    ret.rows.container.emplace_back(IR2ListingLinePair{line.get(), static_cast<u32>(ret.text.size())});
    ret.text.emplace_back(format(*line));
    const auto it = addresses.find(line.get());
    const u64 size = it == addresses.cend() || object_code_suppressed(*line) ? 0 : it->second.size;
    // The first row always prints, even without object code. Each further row continues the object code.
    for (u64 printed = bytes_per_line; printed < size; printed += bytes_per_line) ret.text.emplace_back();
  }
  std::sort(ret.rows.container.begin(), ret.rows.container.end(), IR2ListingLineComparator{});
  return ret;
}

} // namespace pepp::tc
