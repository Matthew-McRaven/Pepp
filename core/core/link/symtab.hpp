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
#include <string_view>
#include <utility>
#include "core/formats/elf/packed_access_symbol.hpp"
#include "core/formats/elf/packed_ops.hpp"
#include "core/link/link_view.hpp"
#include "core/link/symbol_definitions.hpp"

namespace pepp::link {

// Write .symtab and .strtab with one section symbol per output section, then each live file's locals, then every
// defined non-local.
// TODO: write undefined weak and common symbols.
template <ElfBits B, ElfEndian E>
void write_symtab(bts::PackedGrowableElfFile<B, E> &elf, const LinkView<B, E> &link) {
  using namespace bts;
  // Append strtab/symtab to the end of the output.
  const auto outputs_end = static_cast<u16>(elf.section_headers.size());
  const auto strtab = add_named_section(elf, ".strtab", SectionTypes::SHT_STRTAB);
  PackedSymbolWriter<B, E> writer(elf, add_named_symtab(elf, ".symtab", strtab));
  // Copy an input symbol into the output with the address in at, if its section was written.
  const auto copy = [&](PackedElfSymbol<B, E> symbol, std::string_view name, OutputSymbolDefinition at) {
    if (!at) return;
    const u16 section = at.absolute ? u16(symbol.st_shndx) : link.outputs[at.section.value]->shndx;
    // Skip undefined symbols.
    if (section == 0) return;
    symbol.st_value = static_cast<word<B>>(at.address);
    writer.add_symbol(std::move(symbol), name, section);
  };

  // Insert section start symbols.
  for (u16 index = 1; index < outputs_end; ++index) {
    if (index == elf.header.e_shstrndx) continue;
    PackedElfSymbol<B, E> symbol;
    symbol.set_type(SymbolType::STT_SECTION);
    symbol.st_value = elf.section_headers[index].sh_addr;
    writer.add_symbol(std::move(symbol), "", index);
  }

  // Walk all input files and extract their locals. Must not copy non-locals (which include undefineds) because we need
  // to use the linker's resolved symbols instead.
  for (u32 id = 1; id <= link.group.size(); ++id) {
    const ElfFileID file(id);
    const auto index = link.inputs.symtab_of(file);
    if (index == 0 || link.files[id] != Liveness::Live) continue;
    const PackedSymbolReader<B, E> symtab(link.group.file(file), index);
    const u32 first_global = std::min<u32>(link.group.file(file).section_headers[index].sh_info, symtab.symbol_count());
    for (u32 it = 1; it < first_global; ++it) {
      const auto name = symtab.get_symbol_name(it);
      // The output sections' symbols replace input section symbols.
      if (const auto symbol = symtab.get_symbol(it); symbol.st_type() != SymbolType::STT_SECTION)
        copy(symbol, {name.data(), name.size()}, input_symbol_definition(link, file, symbol));
    }
  }

  // Export all of the resolved symbol definitions of the linker, which ought to be non-local.
  for (u32 it = 1; it < link.symbols.size(); ++it) {
    const SymbolID id(it);
    const auto owner = link.symbols.symbol(id).owner;
    // Write a symbol that forwards to another (e.g., foo@V to foo) once, under the other's name.
    if (!owner || link.symbols.of(owner) != id) continue;
    const auto at = link.inputs.handle_of(owner);
    const PackedSymbolReader<B, E> symtab(link.group.file(at.file), link.inputs.symtab_of(at.file));
    auto symbol = symtab.get_symbol(at.index);
    symbol.set_visibility(static_cast<SymbolVisibility>(link.symbols.symbol(id).visibility));
    copy(symbol, link.symbols.name(id), link.definitions[it]);
  }
  // Also sets sh_info. Locals are already first.
  writer.arrange_local_symbols();
}
} // namespace pepp::link
