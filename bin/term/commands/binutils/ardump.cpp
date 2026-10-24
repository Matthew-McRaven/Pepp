/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
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

#include "ardump.hpp"
#include <algorithm>
#include <fmt/format.h>
#include <iostream>
#include <optional>
#include <vector>
#include "core/formats/ar/packed_archive.hpp"
#include "core/math/bitmanip/strings.hpp"

namespace {
using namespace pepp::bts;

void report(const ArchiveError &error) {
  std::cout << fmt::format("error at 0x{:x}: {}\n", error.offset, ArchiveError::to_string(error.code));
}
} // namespace

ArDumpTask::ArDumpTask(Options &opts, QObject *parent) : Task(parent), _opts(opts) {}

void ArDumpTask::run() {
  const auto archive = PackedInputArchive::open(_opts.arfile);
  if (!archive) {
    report(archive.error());
    return emit finished(1);
  }
  std::cout << fmt::format("Archive '{}': {}, {} bytes\n", _opts.arfile, archive->thin() ? "thin" : "regular",
                           archive->size());

  // Walk the members first, so symbols can name the member they point to.
  std::vector<PackedArMember> members;
  std::optional<ArchiveError> error;
  for (const auto &member : archive->members()) {
    if (!member) {
      error = member.error();
      break;
    }
    members.push_back(*member);
  }
  const auto member_at = [&](u64 header_offset) -> std::string_view {
    const auto it = std::ranges::lower_bound(members, header_offset, {}, &PackedArMember::header_offset);
    return it == members.end() || it->header_offset != header_offset ? "<not a member>" : archive->name(*it);
  };

  if (const auto index = archive->symbol_index()) {
    std::cout << fmt::format("\nSymbol index '/' at 0x{:x}: {} bytes\n", index->offset, index->data.size());
    std::cout << fmt::format("  {:>8} {:>8}  {:<24} {}\n", "Entry", "Header", "Symbol", "Member");
    for (const auto &symbol : archive->symbols()) {
      if (!symbol) {
        report(symbol.error());
        break;
      }
      std::cout << fmt::format("  {:>8x} {:>8x}  {:<24} {}\n", symbol->entry_offset, symbol->header_offset,
                               symbol->name, member_at(symbol->header_offset));
    }
  }

  if (const auto long_names = archive->long_names()) {
    const auto table = bits::as_string_view(long_names->data);
    std::cout << fmt::format("\nLong names '//' at 0x{:x}: {} bytes\n", long_names->offset, table.size());
    std::cout << fmt::format("  {:>8}  {}\n", "Offset", "Entry");
    for (size_t start = 0; start < table.size();) {
      const auto end = std::min(table.find('\n', start), table.size());
      // GNU pads the table with a trailing newline.
      if (end > start) std::cout << fmt::format("  {:>8x}  {}\n", start, table.substr(start, end - start));
      start = end + 1;
    }
  }

  std::cout << fmt::format("\nMembers: {}\n", members.size());
  std::cout << fmt::format("  {:>5} {:>8} {:>8} {:>8} {:>10} {:>5} {:>5} {:>6}  {:<16} {}\n", "Index", "Header", "Data",
                           "Size", "Date", "UID", "GID", "Mode", "ar_name", "Name");
  for (size_t it = 0; it < members.size(); ++it) {
    const auto &member = members[it];
    const auto &header = member.header;
    std::cout << fmt::format("  {:>5} {:>8x} {:>8x} {:>8} {:>10} {:>5} {:>5} {:>6o}  {:<16} {}\n", it,
                             member.header_offset, member.data_offset(), member.size, header.date().value_or(0),
                             header.uid().value_or(0), header.gid().value_or(0), header.mode().value_or(0),
                             header.raw_name(), archive->name(member));
  }

  if (error) {
    report(*error);
    return emit finished(1);
  }
  return emit finished(0);
}
