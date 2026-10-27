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
#include <utility>
#include <variant>
#include <vector>
#include "core/formats/elf/packed_input_group.hpp"
#include "core/link/symbol_resolution.hpp"

namespace pepp::link {
// The state of a link that is shared between passes. The associate InputGroup must outlive this context.
struct LinkContext {
  InputSymbols inputs;
  // Indexed by file ID. Index 0 is unused.
  std::vector<Liveness> files;
  ResolvedSymbols symbols;
};

// For each file ID of the group, Live if not an archive, or Lazy if an archive member.
template <bts::ElfBits B, bts::ElfEndian E>
std::vector<Liveness> default_liveness(const bts::PackedInputElfGroup<B, E> &group) {
  std::vector<Liveness> ret(group.size() + 1);
  for (u32 file = 1; file <= group.size(); ++file) {
    const bool member = std::holds_alternative<bts::ElfFromArchive>(group.provenance(bts::ElfFileID(file)));
    ret[file] = member ? Liveness::Lazy : Liveness::Live;
  }
  return ret;
}

// Our equivalent to mold_main, which runs all of the steps of the linking process for a particular context.
// TODO: still missing many steps.
template <bts::ElfBits B, bts::ElfEndian E>
std::expected<LinkContext, GatherError> do_link(const bts::PackedInputElfGroup<B, E> &group) {
  auto inputs = gather_symbols(group);
  if (!inputs) return std::unexpected(inputs.error());

  // As in mold, perform two passes for symbol resolution.
  auto files = default_liveness(group);
  // The first pass, determines the best definition of each symbol, marking archive members as live if they are used by
  // another live file. Without archive members, both passes would agree, so skip the first.
  if (std::ranges::find(files, Liveness::Lazy) != files.end()) {
    mark_live_objects(*inputs, resolve_symbols(*inputs, files), files);
    std::ranges::replace(files, Liveness::Lazy, Liveness::Excluded);
  }
  // TODO: parse and execute the linker script, which may define symbols and place sections.
  // In the second pass, do not include archive members that are now marked as excluded, which helps avoid undefined
  // symbol errors.
  auto symbols = resolve_symbols(*inputs, files);
  return LinkContext{std::move(*inputs), std::move(files), std::move(symbols)};
}
} // namespace pepp::link
