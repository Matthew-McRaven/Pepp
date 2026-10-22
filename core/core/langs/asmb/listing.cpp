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

#include "core/langs/asmb/listing.hpp"
#include <fmt/format.h>
#include <map>
#include "core/formats/elf/enums_sections.hpp"
#include "core/langs/asmb/debug_line.hpp"
#include "core/math/bitmanip/strings.hpp"

namespace {
using namespace pepp::bts;

// The bytes of `row`. A row in SHN_ABS is in the allocated section holding its start, and NOBITS sections have none.
template <ElfBits B, ElfEndian E>
std::expected<bits::span<const u8>, std::string> row_bytes(const PackedElf<B, E> &elf,
                                                          const pepp::tc::ListingRow &row) {
  if (row.end == row.start) return bits::span<const u8>{};
  const bool absolute = row.section == bits::to_underlying(SectionIndices::SHN_ABS);
  const u16 index = absolute ? section_at_address(elf, row.start) : row.section;
  if (index == 0 || index >= elf.section_headers.size())
    return std::unexpected(fmt::format("listing line {} at {:#x} is in no section", row.line, row.start));
  const auto &header = elf.section_headers[index];
  if (header.sh_type == bits::to_underlying(SectionTypes::SHT_NOBITS)) return bits::span<const u8>{};
  // Addresses in the line program are the section's address plus an offset, even in an ET_REL.
  const u64 address = header.sh_addr;
  std::optional<bits::span<const u8>> bytes;
  if (row.start >= address) bytes = section_bytes(elf, index, row.start - address, row.end - row.start);
  if (!bytes) return std::unexpected(fmt::format("listing line {} is outside its section", row.line));
  return *bytes;
}

template <ElfBits B, ElfEndian E>
std::expected<std::vector<std::string>, std::string>
format(const PackedElf<B, E> &elf, const std::vector<std::string> &text, pepp::tc::ListingStyle style) {
  const auto rows = pepp::tc::read_listing_rows(elf);
  if (!rows) return std::unexpected(rows.error());
  // LNP rows are 1-indexed rather than 0-indexed like text.
  std::map<u32, const pepp::tc::ListingRow *> by_line;
  for (const auto &row : *rows)
    if (!by_line.emplace(row.line, &row).second)
      return std::unexpected(fmt::format("listing line {} has more than one row", row.line));

  const int address_width = style.address_width, object_width = 2 * style.bytes_per_row;

  std::vector<char> hex(object_width);
  const auto hex_of = [&](bits::span<const u8> bytes) {
    return std::string(hex.data(), bits::bytesToAsciiHex(hex, bytes, {}));
  };

  // Ensure that every line of the LNP is consumed, otherwise the listing text is inconsistent with the ELF.
  size_t used = 0;

  std::vector<std::string> ret;
  ret.reserve(text.size());

  // Prepend address and object code to each line of the listing text. If the text line does not correspond to a row of
  // the LNP, address+OC are left empty. If OC does not fit on a single line, object code continuation lines are added.
  for (const auto &source : text) {
    const auto row = by_line.find(static_cast<u32>(ret.size() + 1));
    // Row not included in LNP, which means that it is not
    if (row == by_line.end()) {
      ret.emplace_back(fmt::format("{:<{}} {:<{}} {}", "", address_width, "", object_width, source));
      continue;
    } else ++used;
    const auto bytes = row_bytes(elf, *row->second);
    if (!bytes) return std::unexpected(bytes.error());
    const auto first = bytes->first(std::min<size_t>(style.bytes_per_row, bytes->size()));
    const auto address = fmt::format("{:0{}X}", row->second->start, address_width);
    ret.emplace_back(fmt::format("{:<{}} {:<{}} {}", address, address_width, hex_of(first), object_width, source));
    // Create OC continuation rows.
    for (auto rest = bytes->subspan(first.size()); !rest.empty();) {
      const auto chunk = rest.first(std::min<size_t>(style.bytes_per_row, rest.size()));
      ret.emplace_back(fmt::format("{:<{}} {}", "", address_width, hex_of(chunk)));
      rest = rest.subspan(chunk.size());
    }
  }
  if (used != by_line.size())
    return std::unexpected(fmt::format("{} listing line rows do not start a listed line", by_line.size() - used));
  return ret;
}
} // namespace

std::expected<std::vector<std::string>, std::string>
pepp::tc::format_listing(const pepp::bts::AnyInputElf &elf, const std::vector<std::string> &text, ListingStyle style) {
  return std::visit([&](const auto &file) { return format(*file, text, style); }, elf);
}
