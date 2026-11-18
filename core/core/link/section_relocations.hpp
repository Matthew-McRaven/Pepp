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
#include <span>
#include <vector>
#include "core/formats/elf/packed_access_relocations.hpp"
#include "core/formats/elf/packed_access_symbol.hpp"
#include "core/formats/elf/packed_types.hpp"
#include "core/link/diagnostics.hpp"
#include "core/link/link_view.hpp"
#include "core/link/relocations.hpp"
#include "core/link/symbol_definitions.hpp"
#include "core/math/bitmanip/enums.hpp"

namespace pepp::link {

// Resolve an input symbol to a value that can be used for relocation. nullopt if the symbol is not accessible in the
// output. Undefined and weak symbols are 0.
// TODO: are local symbols always ignored as things against which to relocate?
template <ElfBits B, ElfEndian E>
std::optional<u64> relocation_target_address(const LinkView<B, E> &link, const bts::PackedSymbolReader<B, E> &symtab,
                                             bts::ElfFileID file, u32 index) {
  if (index == 0) return 0;
  const auto id = link.inputs.find(file, index);

  // Contains no local definitions
  if (const auto at = input_symbol_definition(link, file, symtab.get_symbol(index)); !id)
    return at ? std::optional(at.address) : std::nullopt;

  const auto resolved = link.symbols.of(*id);
  if (const auto at = link.definitions[resolved.value]) return at.address;
  const bool undefined = !link.symbols.symbol(resolved).owner;
  if (undefined && symtab.get_symbol(index).st_bind() == bts::SymbolBinding::STB_WEAK) return 0;
  return std::nullopt;
}

// Apply relocations to output's copy of the bytes. bytes should be an alias of target's data, just in the output file.
template <ElfBits B, ElfEndian E>
void apply_relocations(Diagnostics &diagnostics, const LinkView<B, E> &link, InputElfSectionHandle relocations,
                       InputElfSectionHandle target, std::span<u8> bytes) {
  const auto &file = link.group.file(target.file);
  if (file.section_headers[relocations.index].sh_type != bits::to_underlying(bts::SectionTypes::SHT_RELA)) {
    diagnostics.add({Severity::Error, RelocationCode::NotRela, RelocationSite{relocations, 0}});
    return;
  }
  const auto machine = static_cast<bts::ElfMachineType>(u16(file.header.e_machine));
  const bts::PackedRelocationReader<B, E> reader(file, static_cast<u16>(relocations.index));
  const bts::PackedSymbolReader<B, E> symtab(file, file.section_headers[relocations.index].sh_link);
  for (u32 it = 0; it < reader.relocation_count(); ++it) {
    const auto rela = reader.get_rela(it);
    const ResolvedRelocation rel{.offset = rela.r_offset,
                                 .type = static_cast<u32>(rela.r_type()),
                                 .symbol = relocation_target_address(link, symtab, target.file, u32(rela.r_sym())),
                                 .addend = static_cast<i64>(bts::sword<B>(rela.r_addend))};
    if (const auto code = apply_relocation(machine, E, rel, bytes))
      diagnostics.add({Severity::Error, *code, RelocationSite{target, rel.offset}});
  }
}

// For each placed input section, the section index of it relocation section in its input file. SHN_UNDEF/0 with no
// relocations.
template <ElfBits B, ElfEndian E> std::vector<u32> relocation_section_indices(const LinkView<B, E> &link) {
  using enum bts::SectionTypes;
  std::vector<u32> ret(link.placements.sections().size(), 0);
  for (u32 id = 1; id <= link.group.size(); ++id) {
    const auto &headers = link.group.file(bts::ElfFileID(id)).section_headers;
    for (u32 index = 0; index < headers.size(); ++index) {
      const auto type = static_cast<bts::SectionTypes>(u32(headers[index].sh_type));
      if (type != SHT_RELA && type != SHT_REL) continue;
      const auto position = link.placements.sections().position({bts::ElfFileID(id), headers[index].sh_info});
      if (position) ret[*position] = index;
    }
  }
  return ret;
}
} // namespace pepp::link
