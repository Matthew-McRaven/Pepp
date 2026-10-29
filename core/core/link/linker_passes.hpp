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
#include <optional>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>
#include "core/formats/elf/packed_input_group.hpp"
#include "core/link/link_config.hpp"
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

struct LinkError {
  enum class Code : u8 {
    // No input provides an ELF file from which to take the class and byte order.
    NoInputs,
    // A config field or input kind that is not implemented yet.
    Unsupported,
    // An input could not be opened or added to the group. See group.
    BadInput,
    // An input's symbol table is malformed. See gather.
    BadSymbols,
  };
  static std::string_view to_string(Code code);

  Code code;
  // If error is specific to a single input file this is its index in LinkConfig::inputs.
  std::optional<u32> input = std::nullopt;
  std::optional<bts::InputGroupError> group = std::nullopt;
  std::optional<GatherError> gather = std::nullopt;
};

// The inputs of a link and the state derived from them. context refers into group, which is heap-allocated.
struct LinkResult {
  bts::AnyElfGroup group;
  LinkContext context;
};

// Our equivalent to mold_main, which runs all of the steps of the linking process for a particular config.
std::expected<LinkResult, LinkError> do_link(LinkConfig &&config);

// The steps of do_link that run once the inputs are in a group.
// TODO: still missing many steps.
template <bts::ElfBits B, bts::ElfEndian E>
std::expected<LinkContext, GatherError> link_group(const bts::PackedInputElfGroup<B, E> &group) {
  /*
   * Symbol resolution passes
   */
  // TODO: handle wrapped symbols, --retain-symbol-files prior to symbol resolution.
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
  // TODO: Merge COMDAT groups.
  // TODO: Create a selection for only-live files/sections.
  // In the second pass, do not include archive members that are now marked as excluded, which helps avoid undefined
  // symbol errors.
  auto symbols = resolve_symbols(*inputs, files);
  /*
   * Inspect special sections
   */
  // TODO: parse .eh_frame, .sframe sections
  // TODO: handle mergeable sections
  if (false /*relocatable*/) {
    // TODO: create linker synthetic sections
    // TODO: mark undefined symbols as imports
    // TODO: compute section sizes, organize output sections to mimimize segment count
    // TODO: construct output symtab / .eh_frame / .sframe
    // TODO: create relocations, COMDAT groups
    // TODO: the great bit-copya
    return {};
  } else {
    // TODO: create .bss sections for commons
    // TODO: compute import / export flags for each symbol
    // TODO: create synthetic sections for the output, synthetic symbols
    // TODO: validate no undefined symbols (if shlib), multiple definitions
    // TODO: garbage collection, identical-code-folding
    // TODO: assign input sections to output sections
    // Output sections are now frozen
    // TODO: handle --section-align for over-aligning sections
    // TODO: mark undefined symbols as imports
    // Symbol tables are now frozen
    // TODO: raise errors for --require-defined poiting to undef symbol.
    // TODO: apply sorting to array sections: .init_array, ._fini_array, .ctors, .dtors
    // TODO: shuffle sections
    // TODO: fill .dynstr from .dynsym
    // TODO: extra got/plt/dynsym/etc entries from relocations
    // TODO: sort output sections to minimize segment count
    // TODO: separate debug info??
    // TODO: compute layout of output sections and construct a map of in->out
    // TODO: linker-generated section contents
    // TODO: emit relocations, symtab, strtab, shstrtab, etc
    // TODO: apply linker relaxation, remove redundant thunks. This can adjust the output layout
    // Memory layout fixed / all symbol addresses are correct
    // TODO: compress relevant sections, ensure thunks are coalesced
    // Memory layout and file layout are fixed
    // TODO: begin the bit copying, apply relocations as we go
    return LinkContext{std::move(*inputs), std::move(files), std::move(symbols)};
  }
}
} // namespace pepp::link
