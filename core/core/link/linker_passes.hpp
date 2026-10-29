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
#include <string>
#include <utility>
#include <variant>
#include <vector>
#include "core/formats/elf/packed_input_group.hpp"
#include "core/link/diagnostics.hpp"
#include "core/link/link_config.hpp"
#include "core/link/symbol_resolution.hpp"

namespace pepp::link {
// The inputs of a link and the state shared between its passes.
struct LinkContext {
  // Null if the link stopped before any input was read.
  bts::AnyElfGroup group;
  Diagnostics diagnostics;
  // Indexed by ConfigInputID. Each input's path, or <memory>.
  std::vector<std::string> input_names;
  InputSymbols inputs;
  // Indexed by file ID. Index 0 is unused.
  std::vector<Liveness> files;
  ResolvedSymbols symbols;

  // True if the link produced no Error or Fatal diagnostics.
  bool ok() const { return diagnostics.count(Severity::Error) == 0; }
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

// Our equivalent to mold_main, which runs all of the steps of the linking process for a particular config.
// Stops at the first Fatal diagnostic. Check ok() before using the result.
LinkContext do_link(LinkConfig &&config);
// Pretty-print a description of a diagnostic message.
std::string describe(const LinkContext &context, const Diagnostic &diagnostic);

// The steps of do_link that run once the inputs are in a group.
// TODO: still missing many steps.
template <bts::ElfBits B, bts::ElfEndian E>
void link_group(LinkContext &context, const bts::PackedInputElfGroup<B, E> &group) {
  auto &diagnostics = context.diagnostics;
  auto &inputs = context.inputs;
  auto &files = context.files;
  /*
   * Symbol resolution passes
   */
  // TODO: handle wrapped symbols, --retain-symbol-files prior to symbol resolution.
  if (auto gathered = gather_symbols(diagnostics, group); !gathered) return;
  else inputs = std::move(*gathered);

  // As in mold, perform two passes for symbol resolution.
  files = default_liveness(group);
  // The first pass, determines the best definition of each symbol, marking archive members as live if they are used by
  // another live file. Without archive members, both passes would agree, so skip the first.
  // Discard this pass's diagnostics so that unused lazy member's undefined references are not errors.
  if (std::ranges::find(files, Liveness::Lazy) != files.end()) {
    Diagnostics discarded;
    mark_live_objects(inputs, resolve_symbols(discarded, inputs, files), files);
    std::ranges::replace(files, Liveness::Lazy, Liveness::Excluded);
  }
  // TODO: Merge COMDAT groups.
  // TODO: Create a selection for only-live files/sections.
  // In the second pass, do not include archive members that are now marked as excluded, which helps avoid undefined
  // symbol errors.
  context.symbols = resolve_symbols(diagnostics, inputs, files);
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
    return;
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
  }
}
} // namespace pepp::link
