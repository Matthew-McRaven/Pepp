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
#include <span>
#include <vector>
#include "core/formats/elf/packed_input_group.hpp"
#include "core/langs/ldscript/ir.hpp"
#include "core/link/section_placements.hpp"
#include "core/link/symbol_resolution.hpp"

namespace pepp::link {

struct OutputSymbolDefinition {
  // Invalid/false for an absolute and undefined symbols.
  OutputSectionID section;
  u64 address = 0;
  bool absolute = false;
  // False for undefined symbols.
  constexpr explicit operator bool() const { return absolute || bool(section); }
};

// LinkContext's state after layout. Must not outlive the LinkContext.
template <ElfBits B, ElfEndian E> struct LinkView {
  const bts::PackedInputElfGroup<B, E> &group;
  // Indexed by file ID.
  std::span<const Liveness> files;
  const InputSymbols &inputs;
  const ResolvedSymbols &symbols;
  const SectionPlacements &placements;
  // Indexed by OutputSectionID. Index 0 is null. Creating the output sets each shndx, so the descriptions cannot be
  // const.
  std::span<tc::ldscript::OutputSectionDescription *const> outputs;
  // Indexed by SymbolID. Set in resolved_symbol_definitions as a view over LinkContext::definitions.
  std::span<const OutputSymbolDefinition> definitions = {};
};
} // namespace pepp::link
