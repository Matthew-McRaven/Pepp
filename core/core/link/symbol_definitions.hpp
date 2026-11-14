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
#include <vector>
#include "core/formats/elf/packed_access_symbol.hpp"
#include "core/link/link_view.hpp"

namespace pepp::link {

// Return the symbol definition in the output file for a given input definition.
// False if undefined, common, or if its' containing section is discarded.
// TODO: common symbols have no address until they are allocated in .bss.
template <ElfBits B, ElfEndian E>
OutputSymbolDefinition input_symbol_definition(const LinkView<B, E> &link, bts::ElfFileID file,
                                               const bts::PackedElfSymbol<B, E> &symbol) {
  if (const u64 value = symbol.st_value; symbol.is_abs()) return {.address = value, .absolute = true};
  else if (symbol.is_undef() || symbol.st_shndx >= bits::to_underlying(bts::SectionIndices::SHN_LORESERVE)) return {};
  // Symbol's file is not live.
  else if (const auto pos = link.placements.sections().position({file, symbol.st_shndx}); !pos) return {};
  // Symbol's section was not assigned to an output section.
  else if (const auto assignment = link.placements.assignments()[*pos]; !assignment) return {};
  // Symbol's section was explicitly discarded.
  else if (const auto &output = *link.outputs[assignment.output.value]; output.discard) return {};
  else return {.section = assignment.output, .address = output.vma + assignment.offset + value};
}

// Compute all symbol definitions in the output, indexed by SymbolID. Must run after layout.
template <ElfBits B, ElfEndian E>
std::vector<OutputSymbolDefinition> resolved_symbol_definitions(const LinkView<B, E> &link) {
  std::vector<OutputSymbolDefinition> ret(link.symbols.size());
  for (u32 it = 1; it < link.symbols.size(); ++it) {
    const auto owner = link.symbols.symbol(SymbolID(it)).owner;
    if (!owner) continue;
    const auto at = link.inputs.handle_of(owner);
    const bts::PackedSymbolReader<B, E> symtab(link.group.file(at.file), link.inputs.symtab_of(at.file));
    ret[it] = input_symbol_definition(link, at.file, symtab.get_symbol(at.index));
  }
  return ret;
}
} // namespace pepp::link
