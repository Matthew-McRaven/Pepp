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
#include <expected>
#include <optional>
#include <string_view>
#include <vector>
#include "core/ds/opaque_handle.hpp"
#include "core/formats/elf/packed_access_symbol.hpp"
#include "core/formats/elf/packed_input_group.hpp"

namespace pepp::link {
// Index of a symbol in a FileSymbolTable. 0 is STN_UNDEF.
using FileSymbolID = pepp::OpaqueHandle<struct FileSymbolTag, u32>;
consteval void allow_opaque_handle_add(FileSymbolID);

struct SymbolTableError {
  enum class Code : u8 {
    // sh_link does not point to a valid string table.
    BadStringTable,
    // sh_info is greater than the number of symbols in the table.
    BadFirstGlobal,
    LocalAfterFirstGlobal,
    GlobalBeforeFirstGlobal,
    // A non-local STT_FILE or STT_SECTION.
    LocalOnlyType,
  };
  static std::string_view to_string(Code code);

  Code code;
  bts::ElfFileID file;
  // The offending symbol index in the given file's symtab. 0 if the error is not specific to a single symbol.
  u32 index = 0;
};

// The non-local symbols of every file in a group, stored as a flat vector sorted by file then symtab order.
// Index 0 is reserved for the undefined symbol.
class FileSymbolTable {
public:
  // Extract all non-local symbols for each ELF file in the group. Files without symbol tables are skipped. Archive's
  // symbol index is ignored, meaning members' symbol tables are re-read from their ELF symtabs.
  template <bts::ElfBits B, bts::ElfEndian E>
  static std::expected<FileSymbolTable, SymbolTableError> collect(const bts::PackedInputElfGroup<B, E> &group);

  // Including STN_UNDEF/0 IDs are [0, size()).
  u32 size() const noexcept { return static_cast<u32>(_names.size()); }
  // Both throw std::out_of_range if id >= size(). Name points into the ELF file's string table storage.
  std::string_view name(FileSymbolID id) const;
  bool undefined(FileSymbolID id) const;
  // The ID of symbol index of file id: STN_UNDEF for index 0, or nullopt if the symbol is local or does not exist.
  std::optional<FileSymbolID> find(bts::ElfFileID id, u32 index) const;
  // The number of files in the group when this table was made.
  u32 files() const noexcept { return static_cast<u32>(_non_locals.size() - 1); }

private:
  // Both are indexed by FileSymbolID. Entry 0 is STN_UNDEF which is both undefined and has no name.
  std::vector<std::string_view> _names = std::vector<std::string_view>(1);
  std::vector<bool> _undefined = std::vector<bool>(1, true);

  // The span of one file's non-local symbols
  struct NonLocalRange {
    // The file's first non-local symbol. All symbols in this file can be expressed as an offset from first_id.
    FileSymbolID first_id;
    // The symbol table's index for the first non-local that is read from sh_info.
    u32 first_global = 0;
    // How many non-local symbols does the file contain?
    u32 count = 0;
  };
  // Indexed by file ID. Index 0 is unused.
  std::vector<NonLocalRange> _non_locals = std::vector<NonLocalRange>(1);
};

template <bts::ElfBits B, bts::ElfEndian E>
std::expected<FileSymbolTable, SymbolTableError>
FileSymbolTable::collect(const bts::PackedInputElfGroup<B, E> &group) {
  using namespace bts;
  using enum SymbolTableError::Code;
  constexpr auto fail = [&](u32 id, SymbolTableError::Code code, u32 index) {
    return std::unexpected(SymbolTableError{code, ElfFileID(id), index});
  };

  FileSymbolTable ret;

  for (u32 id = 1; id <= group.size(); ++id) {
    const auto &elf = group.file(ElfFileID(id));
    NonLocalRange range{.first_id = FileSymbolID(static_cast<u32>(ret._names.size()))};
    // Search for the first (only?) SHT_SYMTAB section. If there is none, the file has no non-local symbols.
    for (u16 sec = 0; sec < elf.section_headers.size(); ++sec) {
      const auto &shdr = elf.section_headers[sec];
      if (shdr.sh_type != bits::to_underlying(SectionTypes::SHT_SYMTAB)) continue;
      else if (shdr.sh_link >= elf.section_headers.size()) return fail(id, BadStringTable, 0);
      const PackedSymbolReader<B, E> symtab(elf, sec);
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
        ret._undefined.push_back(sym.is_undef());
      }
      break;
    }
    range.count = static_cast<u32>(ret._names.size()) - range.first_id.value;
    ret._non_locals.push_back(range);
  }
  return ret;
}
} // namespace pepp::link
