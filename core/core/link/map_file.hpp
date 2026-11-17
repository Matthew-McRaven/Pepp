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
#include <optional>
#include <string>
#include <vector>
#include "core/formats/elf/packed_access_headers.hpp"
#include "core/formats/elf/packed_access_symbol.hpp"
#include "core/formats/elf/packed_input_group.hpp"
#include "core/formats/elf/packed_ops.hpp"
#include "core/link/file_name.hpp"
#include "core/link/link_view.hpp"

namespace pepp::link {
struct LinkContext;

// Duplicate input data needed by link_map to allow mapfile usage after input files are closed.
struct InputSummary {
  ElfBits bits = ElfBits::b32;
  // Indexed by file ID. Index 0 is unused.
  std::vector<std::string> file_names;
  // All are indexed like SectionPlacements::sections().
  std::vector<std::string> section_names;
  std::vector<u64> section_sizes;
  // The address of each input section in the output, or nullopt if it was not placed (e.g., it was discarded).
  std::vector<std::optional<u64>> section_addresses;
  // Indexed by SymbolID. The position of the input section defining each symbol. nullopt if undefined or absolute.
  std::vector<std::optional<u32>> defined_in;
};

// Construct an input summary. Must run after resolved_symbol_definitions and before close_input.
template <ElfBits B, ElfEndian E> InputSummary summarize_inputs(const LinkView<B, E> &link) {
  InputSummary ret{.bits = B};
  ret.file_names.resize(link.group.size() + 1);
  for (u32 id = 1; id <= link.group.size(); ++id) ret.file_names[id] = file_name(link.group, bts::ElfFileID(id));

  const auto &selection = link.placements.sections();
  const auto assignments = link.placements.assignments();
  ret.section_names.reserve(selection.size()), ret.section_sizes.reserve(selection.size());
  ret.section_addresses.resize(selection.size());
  for (u32 position = 0; position < selection.size(); ++position) {
    const auto handle = selection.handle(position);
    const auto &file = link.group.file(handle.file);
    ret.section_names.emplace_back(bts::section_name(file, handle.index));
    ret.section_sizes.push_back(bts::word<B>(file.section_headers[handle.index].sh_size));
    if (const auto assignment = assignments[position]; assignment && !link.outputs[assignment.output.value]->discard)
      ret.section_addresses[position] = link.outputs[assignment.output.value]->vma + assignment.offset;
  }

  ret.defined_in.resize(link.symbols.size());
  for (u32 it = 1; it < link.symbols.size(); ++it) {
    const auto owner = link.symbols.symbol(SymbolID(it)).owner;
    if (const auto definition = link.definitions[it]; !owner || !definition || definition.absolute) continue;
    const auto at = link.inputs.handle_of(owner);
    const bts::PackedSymbolReader<B, E> symtab(link.group.file(at.file), link.inputs.symtab_of(at.file));
    ret.defined_in[it] = selection.position({at.file, symtab.get_symbol(at.index).st_shndx});
  }
  return ret;
}

// Returns the "Linker script and memory map" part of GNU ld's -Map, or nullopt if the link stopped before layout.
// TODO: list assignments, the memory configuration, the members of /DISCARD/, and absolute symbols (e.g., from -R).
std::optional<std::string> link_map(const LinkContext &context);

// Returns the address of an input section in the output. nullopt if the section was not assigned.
std::optional<u64> section_address(const LinkContext &context, bts::InputElfSectionHandle section);
} // namespace pepp::link
