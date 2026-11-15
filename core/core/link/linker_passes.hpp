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
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>
#include "core/formats/elf/packed_input_group.hpp"
#include "core/link/diagnostics.hpp"
#include "core/link/entry.hpp"
#include "core/link/link_config.hpp"
#include "core/link/link_context.hpp"
#include "core/link/link_view.hpp"
#include "core/link/output_file.hpp"
#include "core/link/section_layout.hpp"
#include "core/link/section_placements.hpp"
#include "core/link/symbol_definitions.hpp"
#include "core/link/symbol_resolution.hpp"

namespace pepp::link {

// Returns Lazy for each archive member and Live for every other file, indexed by file ID.
template <bts::ElfBits B, bts::ElfEndian E>
std::vector<Liveness> default_liveness(const bts::PackedInputElfGroup<B, E> &group) {
  std::vector<Liveness> ret(group.size() + 1);
  for (u32 file = 1; file <= group.size(); ++file) {
    const bool member = std::holds_alternative<bts::ElfFromArchive>(group.provenance(bts::ElfFileID(file)));
    ret[file] = member ? Liveness::Lazy : Liveness::Live;
  }
  return ret;
}

// Run every step of the linker for a given config, stopping at the first Fatal diagnostic.
// Check ok() before using the result.
LinkContext do_link(LinkConfig &&config);
// Format a diagnostic as a message for the user.
std::string describe(const LinkContext &context, const Diagnostic &diagnostic);

// Mark the output at path as executable if the link has no errors. Otherwise, release the output and delete it.
void close_output(LinkContext &context, const std::optional<std::filesystem::path> &path);

// The steps of do_link that run once the inputs are in a group.
// TODO: still missing many steps.
template <bts::ElfBits B, bts::ElfEndian E>
void link_group(LinkContext &context, const bts::PackedInputElfGroup<B, E> &group, const LinkConfig &config) {
  auto &diagnostics = context.diagnostics;
  auto &inputs = context.inputs;
  auto &files = context.files;
  /*
   * Symbol resolution passes
   */
  // TODO: handle wrapped symbols, --retain-symbol-files prior to symbol resolution.
  if (auto gathered = gather_symbols(diagnostics, group); !gathered) return;
  else inputs = std::move(*gathered);

  // Resolve symbols in two passes.
  files = default_liveness(group);
  // The first pass picks the best definition of each symbol and marks an archive member live if a live file uses it.
  // Only archive members can be lazy, so this pass can be skipped without archives. Discard this pass's diagnostics so
  // that an unused lazy member's undefined references are not errors.
  if (std::ranges::find(files, Liveness::Lazy) != files.end()) {
    Diagnostics discarded;
    mark_live_objects(inputs, resolve_symbols(discarded, inputs, files), files);
    std::ranges::replace(files, Liveness::Lazy, Liveness::Excluded);
  }
  // TODO: Merge COMDAT groups.
  // The second pass leaves out the archive members excluded by the first pass, so their undefined references are not
  // errors.
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
    // Needed so that orphans can be inserted among assignments from any SECTIONS command.
    auto &sections = tc::ldscript::merge_sections(context.script);
    // Assign the live files' sections to the output sections, inserting output sections for orphans.
    context.placements = place_sections(group, files, sections);
    /*Output sections are now frozen*/
    // TODO: handle --section-align for over-aligning sections
    // TODO: mark undefined symbols as imports
    /*Symbol tables are now frozen*/
    // TODO: raise errors for --require-defined poiting to undef symbol.
    // TODO: apply sorting to array sections: .init_array, ._fini_array, .ctors, .dtors
    // TODO: shuffle sections
    // TODO: fill .dynstr from .dynsym
    // TODO: extra got/plt/dynsym/etc entries from relocations
    // TODO: sort output sections to minimize segment count
    // TODO: separate debug info??
    // Assign addresses to the output sections and offsets to their members.
    lay_out_sections(diagnostics, group, *context.placements, sections);
    // TODO: linker-generated section contents
    // TODO: emit relocations, symtab, strtab, shstrtab, etc
    // TODO: apply linker relaxation, remove redundant thunks. This can adjust the output layout
    /*Memory layout fixed / all symbol addresses are correct*/
    // TODO: compress relevant sections, ensure thunks are coalesced
    /*Memory layout and file layout are fixed*/
    // Compute the file layout, copy the input sections into the output file, then apply relocations there.
    if (!context.ok()) return;
    const auto outputs = output_sections(sections);
    LinkView<B, E> link{group, files, inputs, context.symbols, *context.placements, outputs};
    link.definitions = resolved_symbol_definitions(link);
    const auto entry = entry_address(diagnostics, link, context.script, config.entry, sections);
    context.output = create_executable_output(diagnostics, link, sections, entry, config.output);
    close_output(context, config.output);
  }
}
} // namespace pepp::link
