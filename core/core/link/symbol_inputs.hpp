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
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>
#include "core/formats/elf/packed_access_symbol.hpp"
#include "core/formats/elf/packed_input_group.hpp"
#include "core/link/diagnostics.hpp"

namespace pepp::link {
// The attributes of a non-local symbol read by resolution.
struct SymbolAttributes {
  u8 undefined : 1 = 0;
  u8 weak : 1 = 0;
  u8 common : 1 = 0;
  u8 tls : 1 = 0;
  // STT_NOTYPE, which says nothing about whether the symbol is thread-local.
  u8 untyped : 1 = 0;
  // An STV_* value.
  u8 visibility : 2 = 0;
};
static_assert(sizeof(SymbolAttributes) == 1);

// A common (SHN_COMMON) definition, whose st_value holds its alignment.
struct CommonSize {
  u64 size = 0;
  u64 alignment = 0;
  bool operator==(const CommonSize &) const = default;
};

// The non-local symbols of every file in a group, stored as a flat vector sorted by file then symtab order.
// Index 0 is reserved for the undefined symbol.
class InputSymbols {
public:
  // IDs are [0, size()), with 0 as STN_UNDEF.
  u32 size() const noexcept { return static_cast<u32>(_names.size()); }
  // All throw std::out_of_range if id >= size(). Name stays valid after the group is released.
  std::string_view name(InputSymbolID id) const;
  bool undefined(InputSymbolID id) const { return attributes(id).undefined; }
  SymbolAttributes attributes(InputSymbolID id) const;
  // Throws std::out_of_range unless id is a common definition.
  CommonSize common(InputSymbolID id) const;
  // Returns the ID of symbol index of file id, which is STN_UNDEF for index 0. Returns nullopt if the symbol is local
  // or does not exist.
  std::optional<InputSymbolID> find(bts::ElfFileID id, u32 index) const;
  // Returns the file and symbol table index of symbol id. Throws std::out_of_range for STN_UNDEF or if id >= size().
  bts::InputElfSymbolHandle handle_of(InputSymbolID id) const;
  // Returns the section index of a file's symbol table, or 0 if it has none.
  u16 symtab_of(bts::ElfFileID id) const;
  // Returns the file that owns symbol id. Throws std::out_of_range for STN_UNDEF, which belongs to every file, or if id
  // >= size().
  bts::ElfFileID file_of(InputSymbolID id) const;
  // Returns the IDs of a file's non-local symbols as [first, last). Throws std::out_of_range if no file has this ID.
  std::pair<InputSymbolID, InputSymbolID> ids_of(bts::ElfFileID id) const;
  // The number of files in the group when this table was made.
  u32 files() const noexcept { return static_cast<u32>(_non_locals.size() - 1); }

private:
  // Both are indexed by InputSymbolID. Entry 0 is STN_UNDEF which is both undefined and has no name.
  std::vector<std::string> _names = std::vector<std::string>(1);
  std::vector<SymbolAttributes> _attributes = std::vector<SymbolAttributes>(1, SymbolAttributes{.undefined = 1});

  // The span of one file's non-local symbols
  struct NonLocalRange {
    // The file's first non-local symbol. All symbols in this file can be expressed as an offset from first_id.
    InputSymbolID first_id;
    // The symbol table's index for the first non-local that is read from sh_info.
    u32 first_global = 0;
    // The number of non-local symbols in the file.
    u32 count = 0;
    // The section index of the file's symbol table, or 0 if it has none.
    u16 symtab = 0;
  };
  // Indexed by file ID. Index 0 is unused.
  std::vector<NonLocalRange> _non_locals = std::vector<NonLocalRange>(1);
  // Sparse since few symbols are common.
  std::unordered_map<InputSymbolID, CommonSize, pepp::handle_hash<InputSymbolID>> _commons;

  template <bts::ElfBits B, bts::ElfEndian E>
  friend std::optional<InputSymbols> gather_symbols(Diagnostics &diagnostics,
                                                    const bts::PackedInputElfGroup<B, E> &group);
};

// Extract the non-local symbols of each ELF file in the group, skipping files without a symbol table. Ignores each
// archive's symbol index and reads members' own symbol tables instead. A malformed symbol table is Fatal and cites the
// offending symbol index, or 0 for the table as a whole.
template <bts::ElfBits B, bts::ElfEndian E>
std::optional<InputSymbols> gather_symbols(Diagnostics &diagnostics, const bts::PackedInputElfGroup<B, E> &group) {
  using namespace bts;
  using enum SymbolCode;
  const auto fail = [&](u32 id, SymbolCode code, u32 index) -> std::optional<InputSymbols> {
    diagnostics.add({Severity::Fatal, code, InputElfSymbolHandle{ElfFileID(id), index}});
    return std::nullopt;
  };

  InputSymbols ret;

  for (u32 id = 1; id <= group.size(); ++id) {
    const auto &elf = group.file(ElfFileID(id));
    InputSymbols::NonLocalRange range{.first_id = InputSymbolID(static_cast<u32>(ret._names.size()))};
    // Search for the first (only?) SHT_SYMTAB section. If there is none, the file has no non-local symbols.
    for (u16 sec = 0; sec < elf.section_headers.size(); ++sec) {
      const auto &shdr = elf.section_headers[sec];
      if (shdr.sh_type != bits::to_underlying(SectionTypes::SHT_SYMTAB)) continue;
      else if (shdr.sh_link >= elf.section_headers.size()) return fail(id, BadStringTable, 0);
      const PackedSymbolReader<B, E> symtab(elf, sec);
      range.symtab = sec;
      // The null symbol at index 0 is always local. In that case, round up to 1.
      range.first_global = std::max<u32>(shdr.sh_info, 1);
      if (range.first_global > symtab.symbol_count()) return fail(id, BadFirstGlobal, 0);
      // Index 0 is the reserved null symbol.
      for (u32 it = 1; it < symtab.symbol_count(); ++it) {
        const auto sym = symtab.get_symbol(it);
        const bool local = sym.st_bind() == SymbolBinding::STB_LOCAL;
        if (it < range.first_global) {
          if (!local) return fail(id, GlobalBeforeFirstGlobal, it);
          continue;
        } else if (local) return fail(id, LocalAfterFirstGlobal, it);
        else if (sym.st_type() == SymbolType::STT_FILE || sym.st_type() == SymbolType::STT_SECTION)
          return fail(id, LocalOnlyType, it);
        const auto name = symtab.get_symbol_name(it);
        ret._names.emplace_back(name.data(), name.size());
        ret._attributes.push_back({.undefined = static_cast<u8>(sym.is_undef()),
                                   .weak = static_cast<u8>(sym.st_bind() == SymbolBinding::STB_WEAK),
                                   .common = static_cast<u8>(sym.is_common()),
                                   .tls = static_cast<u8>(sym.st_type() == SymbolType::STT_TLS),
                                   .untyped = static_cast<u8>(sym.st_type() == SymbolType::STT_NOTYPE),
                                   .visibility = static_cast<u8>(sym.st_visibility())});
        if (sym.is_common())
          ret._commons.emplace(InputSymbolID(static_cast<u32>(ret._names.size() - 1)),
                               CommonSize{.size = sym.st_size, .alignment = sym.st_value});
      }
      break;
    }
    range.count = static_cast<u32>(ret._names.size()) - range.first_id.value;
    ret._non_locals.push_back(range);
  }
  return ret;
}
} // namespace pepp::link