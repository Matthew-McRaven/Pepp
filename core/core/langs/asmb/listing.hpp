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
#include <expected>
#include <functional>
#include <string>
#include <vector>
#include "core/compile/ir_linear/attr_comment.hpp"
#include "core/formats/elf/packed_access_headers.hpp"
#include "core/integers.h"
#include "core/langs/asmb/elf_symtab.hpp"
#include "core/langs/asmb/ir_program.hpp"

namespace pepp::tc {

// A listing's rows before addresses and object code are filled in, which come from the ELF.
struct ListingLayout {
  // Source-column text of each listed line, excluding object-code continuations.
  std::vector<std::string> text;
  // Each listed line's first printed row, 0-based. Continuation rows are not stored, but their impacts on subsequent
  // line numbering is accounted for.
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
  u32 row = 0;
  for (const auto &line : source_order) {
    if (hidden_from_listing(*line)) continue;
    ret.rows.container.emplace_back(IR2ListingLinePair{line.get(), row});
    ret.text.emplace_back(format(*line));
    const auto it = addresses.find(line.get());
    const u64 size = it == addresses.cend() || object_code_suppressed(*line) ? 0 : it->second.size;
    // The first row always prints, even without object code. Each further row continues the object code.
    row += std::max<u64>(1, (size + bytes_per_line - 1) / bytes_per_line);
  }
  std::sort(ret.rows.container.begin(), ret.rows.container.end(), IR2ListingLineComparator{});
  return ret;
}

struct ListingStyle {
  u8 address_width;
  u32 bytes_per_row;
};

// Formats the lines of a ListingLayout's `text`, adding object code continuation rows where necessary.
// Addresses and object code are extracted from the ELF file. Fails if the line number program does not match the length
// of the text, or if the ELF does not contain the expected address/offsets.
std::expected<std::vector<std::string>, std::string>
format_listing(const pepp::bts::AnyInputElf &elf, const std::vector<std::string> &text, ListingStyle style);

} // namespace pepp::tc
