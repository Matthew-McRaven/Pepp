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

#include "core/formats/elf/managed_section_reloc.hpp"
#include <stdexcept>
#include <string>
#include "core/formats/elf/managed_elf.hpp"
#include "core/formats/elf/packed_types.hpp"

void pepp::bts::ManagedRelocTable::add(ManagedRelocation entry) {
  if (!entry.symbol) throw std::logic_error("ManagedRelocTable: a relocation must name a symbol to resolve");
  _entries.push_back(std::move(entry));
}

pepp::bts::uxword pepp::bts::ManagedRelocTable::file_bytes(ElfBits bits) const {
  return _entries.size() * rela_bytes(bits);
}

pepp::bts::ManagedRelocTable &pepp::bts::relocations_for(ManagedElf &elf, SectionRef target, SectionRef symtab) {
  auto *relocated = elf.section(target);
  if (!relocated) throw std::logic_error("relocations_for: no such section to relocate");
  auto *table = elf.section(symtab);
  if (!table || !table->content_as<ManagedSymbolTable>())
    throw std::logic_error("relocations_for: relocations must resolve through a symbol table");

  // Return the section if it already exists
  for (auto it = ManagedElf::SHN_UNDEF; it <= elf.last_section(); ++it) {
    auto *sec = elf.section(it);
    auto *existing = sec ? sec->content_as<ManagedRelocTable>() : nullptr;
    if (existing && std::get_if<SectionRef>(&sec->info) && std::get<SectionRef>(sec->info) == target)
      return *existing;
  }

  // Othewerwise create a new section and return it
  auto *sec = elf.section(elf.add_section(".rela" + relocated->name, SectionTypes::SHT_RELA));
  sec->link = symtab; // the symbols these resolve through
  sec->info = target; // the section they patch
  sec->addralign = word_bytes(elf.bits()); // Entries are three words wide, so readers expect them word-aligned.
  // Says sh_info holds a section index rather than a count. Not in TIS 1.2, but in the gABI that superseded it.
  sec->flags = SectionFlags::SHF_INFO_LINK;
  return sec->make_content<ManagedRelocTable>();
}
