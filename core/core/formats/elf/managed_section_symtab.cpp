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

#include "core/formats/elf/managed_section_symtab.hpp"
#include <algorithm>
#include <stdexcept>
#include "core/compile/symbol/entry.hpp"
#include "core/compile/symbol/types.hpp"
#include "core/compile/symbol/value.hpp"
#include "core/formats/elf/managed_elf.hpp"
#include "core/formats/elf/managed_section_strtab.hpp"
#include "core/formats/elf/packed_types.hpp"

pepp::bts::ManagedSymbolTable::ManagedSymbolTable(std::shared_ptr<core::symbol::LeafTable> symbols)
    : _symbols(std::move(symbols)) {
  if (!_symbols) throw std::logic_error("ManagedSymbolTable: needs a symbol table to describe");
}

void pepp::bts::ManagedSymbolTable::set_section(const entry_ptr_t &entry, SectionRef ref) {
  if (!entry) throw std::logic_error("ManagedSymbolTable: cannot place a null symbol in a section");
  _sections[entry] = ref;
}

pepp::bts::SectionRef pepp::bts::ManagedSymbolTable::section_of(const entry_ptr_t &entry) const noexcept {
  const auto found = _sections.find(entry);
  return found == _sections.end() ? SectionRef{} : found->second;
}

namespace {
// Do not serialize deleted symbols.
bool is_tombstone(const pepp::bts::ManagedSymbolTable::entry_ptr_t &entry) {
  using namespace pepp::core::symbol;
  return entry && entry->value && entry->value->type() == Type::Deleted;
}
} // namespace

void pepp::bts::ManagedSymbolTable::freeze(const std::function<bool(const entry_ptr_t &)> &keep) {
  using namespace core::symbol;
  const auto &entries = _symbols->entries();
  auto &ordered = _frozen.emplace();
  // Over-allocates when keep garbage collects symbols, but prevents allocations during the insert step.
  ordered.reserve(entries.size() + 1);
  ordered.push_back(nullptr); // Meet ELF requirement that null symbol is first.
  for (const auto &[_, entry] : entries) {
    if (!entry || is_tombstone(entry)) continue;
    else if (keep && !keep(entry)) continue;
    else ordered.push_back(entry);
  }

  // Do not sort the null symbol. Ensure all locals precede any non-local.
  std::sort(ordered.begin() + 1, ordered.end(), [](const entry_ptr_t &lhs, const entry_ptr_t &rhs) {
    const bool left_local = lhs->binding == Binding::Local, right_local = rhs->binding == Binding::Local;
    if (left_local != right_local) return left_local;
    return lhs->name < rhs->name;
  });
}

void pepp::bts::freeze_symbols(ManagedSection &sec, ElfBits bits,
                               const std::function<bool(const ManagedSymbolTable::entry_ptr_t &)> &keep) {
  auto *table = sec.content_as<ManagedSymbolTable>();
  if (!table) throw std::logic_error("freeze_symbols: this section holds no symbol table");
  table->freeze(keep);
  sec.info = u32{table->first_nonlocal()};
  // GNU as likes to align this to word size so that you can mmap and cast easily.
  sec.addralign = word_bytes(bits);
}

std::span<const pepp::bts::ManagedSymbolTable::entry_ptr_t> pepp::bts::ManagedSymbolTable::frozen_order() const {
  if (!_frozen) throw std::logic_error("ManagedSymbolTable: symbol order was never frozen");
  return *_frozen;
}

u32 pepp::bts::ManagedSymbolTable::first_nonlocal() const {
  const auto ordered = frozen_order();
  const auto cmp = [](const auto &entry) { return entry && entry->binding != core::symbol::Binding::Local; };
  const auto local = std::find_if(ordered.begin() + 1, ordered.end(), cmp); // Skip inital null symbol.
  return static_cast<u32>(std::distance(ordered.begin(), local));
}

pepp::bts::uxword pepp::bts::ManagedSymbolTable::file_bytes(ElfBits bits) const {
  return frozen_order().size() * symbol_bytes(bits);
}

void pepp::bts::build_strtabs_for_symtabs(ManagedElf &elf, std::vector<SectionRef> &live) {
  // While we may be appending to live, we will never add a new symbol table. Iteration will terminate.
  for (std::size_t it = 0; it < live.size(); ++it) {
    auto *sec = elf.section(live[it]);
    if (!sec) continue;
    auto *symbols = sec->content_as<ManagedSymbolTable>();
    if (!symbols) continue;

    if (!sec->link) {
      // A dynamic symbol table's names conventionally live in .dynstr rather than .strtab.
      const bool dynamic = sec->type == SectionTypes::SHT_DYNSYM;
      sec->link = elf.add_section(dynamic ? ".dynstr" : ".strtab", SectionTypes::SHT_STRTAB);
    }

    auto *linked = elf.section(sec->link);
    if (!linked) throw std::logic_error("build_strtabs_for_symtabs: a symbol table links to no section");
    else if (linked->type != SectionTypes::SHT_STRTAB)
      throw std::logic_error("build_strtabs_for_symtabs: a symbol table's sh_link is not a string table");

    // Ensure ManagedStringTable exists. Do not clear it if pointing to an existing table
    auto *names = linked->content_as<ManagedStringTable>();
    if (!names) names = &linked->make_content<ManagedStringTable>();

    for (const auto &entry : symbols->frozen_order())
      if (entry) names->insert(entry->name);

    // Ensure string table will be serialized
    if (std::find(live.begin(), live.end(), sec->link) == live.end()) live.push_back(sec->link);
  }
}
