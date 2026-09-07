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
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <unordered_map>
#include <vector>
#include "core/compile/symbol/leaf_table.hpp"
#include "core/formats/elf/managed_section.hpp"
#include "core/formats/elf/managed_types.hpp"

namespace pepp::bts {

class ManagedElf;

/*
 * Represents a SHT_SYMTAB or SHT_DYNSYM section. It reuses the compiler's symbol table up until serialization, since
 * symbol::Entry already contain most of the fields that we need. The only additional per-symbol data that we need to
 * store is the st_shndx field, which tells us the section that defined the symbol. This allows the compiler to be
 * agnostic to our object file format.
 *
 * Operations which need to know the size /ordering of the table require that freeze_symbols(), which performs a
 * combined garbage-collection and sorting pass.
 */
class ManagedSymbolTable : public ManagedPayload {
public:
  using entry_ptr_t = core::symbol::LeafTable::entry_ptr_t;

  explicit ManagedSymbolTable(std::shared_ptr<core::symbol::LeafTable> symbols);

  const core::symbol::LeafTable &symbols() const noexcept { return *_symbols; }
  core::symbol::LeafTable &symbols() noexcept { return *_symbols; }

  void set_section(const entry_ptr_t &entry, SectionRef ref);
  SectionRef section_of(const entry_ptr_t &entry) const noexcept;

  // True if freeze_symbols has been called on this symbol table
  bool is_frozen() const noexcept { return _frozen.has_value(); }
  // The frozen order, with index 0 being the null symbol.
  std::span<const entry_ptr_t> frozen_order() const;
  // For compatibility with real assembler, sh_info must hold the first non-local symbol.
  u32 first_nonlocal() const;
  // Throws unless frozen.
  uxword file_bytes(ElfBits bits) const override;

private:
  friend void freeze_symbols(ManagedSection &sec, ElfBits bits, const std::function<bool(const entry_ptr_t &)> &keep);
  // Actual implementation which creates & sorts the _frozen vector.
  void freeze(const std::function<bool(const entry_ptr_t &)> &keep);

  std::shared_ptr<core::symbol::LeafTable> _symbols;
  std::unordered_map<entry_ptr_t, SectionRef> _sections;
  std::optional<std::vector<entry_ptr_t>> _frozen;
};

/*
 * Combined garbage collection and sorting. ELF requires local symbols be ordered before non-locals, and multiple
 * compiler flags modify which symbols defined by the assembler are written to the output symtab. The predicate
 * determines which symbols are garbage collected; if nullptr all symbols are retained. The set of retained symbols is
 * cached on the ManagedSymbolTable to avoid re-computing the order on each use. This manual garbage collection step
 * allows us to shrink the size of the generated string table.
 *
 * Index 0 of the frozen list is the always-present null symbol, and the section's sh_info will have the index of the
 * first non-local symbol.
 */
void freeze_symbols(ManagedSection &sec, ElfBits bits,
                    const std::function<bool(const ManagedSymbolTable::entry_ptr_t &)> &keep = nullptr);

/* Search the live sections for symbol tables and build their string tables. If no string table exists for a given
 * symbol table, a string table will be created. Newly-added string tables will be added to the live list.
 * Requires the symbol table be frozen to avoid allocating strings for symbols that will be removed.
 */
void build_strtabs_for_symtabs(ManagedElf &elf, std::vector<SectionRef> &live);

} // namespace pepp::bts
