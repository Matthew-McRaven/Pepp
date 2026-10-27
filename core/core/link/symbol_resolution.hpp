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
#include <span>
#include <string_view>
#include <vector>
#include "core/ds/opaque_handle.hpp"
#include "core/formats/elf/packed_access_symbol.hpp"
#include "core/formats/elf/packed_input_group.hpp"

namespace pepp::link {
// Index of a symbol in InputSymbols. 0 is STN_UNDEF.
using InputSymbolID = pepp::OpaqueHandle<struct InputSymbolTag, u32>;
consteval void allow_opaque_handle_add(InputSymbolID);

struct GatherError {
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

// What resolution needs to know about one non-local symbol, read once while gathering. One byte.
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

// The non-local symbols of every file in a group, stored as a flat vector sorted by file then symtab order.
// Index 0 is reserved for the undefined symbol.
class InputSymbols {
public:
  // Including STN_UNDEF/0 IDs are [0, size()).
  u32 size() const noexcept { return static_cast<u32>(_names.size()); }
  // All throw std::out_of_range if id >= size(). Name points into the ELF file's string table storage.
  std::string_view name(InputSymbolID id) const;
  bool undefined(InputSymbolID id) const { return attributes(id).undefined; }
  SymbolAttributes attributes(InputSymbolID id) const;
  // The ID of symbol index of file id: STN_UNDEF for index 0, or nullopt if the symbol is local or does not exist.
  std::optional<InputSymbolID> find(bts::ElfFileID id, u32 index) const;
  // The file that symbol id belongs to. Throws std::out_of_range for STN_UNDEF, which belongs to every file, or if
  // id >= size().
  bts::ElfFileID file_of(InputSymbolID id) const;
  // The number of files in the group when this table was made.
  u32 files() const noexcept { return static_cast<u32>(_non_locals.size() - 1); }

private:
  // Both are indexed by InputSymbolID. Entry 0 is STN_UNDEF which is both undefined and has no name.
  std::vector<std::string_view> _names = std::vector<std::string_view>(1);
  std::vector<SymbolAttributes> _attributes = std::vector<SymbolAttributes>(1, SymbolAttributes{.undefined = 1});

  // The span of one file's non-local symbols
  struct NonLocalRange {
    // The file's first non-local symbol. All symbols in this file can be expressed as an offset from first_id.
    InputSymbolID first_id;
    // The symbol table's index for the first non-local that is read from sh_info.
    u32 first_global = 0;
    // How many non-local symbols does the file contain?
    u32 count = 0;
  };
  // Indexed by file ID. Index 0 is unused.
  std::vector<NonLocalRange> _non_locals = std::vector<NonLocalRange>(1);

  template <bts::ElfBits B, bts::ElfEndian E>
  friend std::expected<InputSymbols, GatherError> gather_symbols(const bts::PackedInputElfGroup<B, E> &group);
};

// Extract all non-local symbols for each ELF file in the group. Files without symbol tables are skipped. Archive's
// symbol index is ignored, meaning members' symbol tables are re-read from their ELF symtabs.
template <bts::ElfBits B, bts::ElfEndian E>
std::expected<InputSymbols, GatherError> gather_symbols(const bts::PackedInputElfGroup<B, E> &group) {
  using namespace bts;
  using enum GatherError::Code;
  constexpr auto fail = [&](u32 id, GatherError::Code code, u32 index) {
    return std::unexpected(GatherError{code, ElfFileID(id), index});
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
      }
      break;
    }
    range.count = static_cast<u32>(ret._names.size()) - range.first_id.value;
    ret._non_locals.push_back(range);
  }
  return ret;
}

// Identifies one symbol of the link after mergin InputSymbols. 0 remains STN_UNDEF
using SymbolID = pepp::OpaqueHandle<struct SymbolTag, u32>;

// Resolved definition of a single symbol.
struct Symbol {
  // The strongest definition. Refer to the owner's attributes for information other than visibility.
  InputSymbolID owner;
  // Record the most constraining visibility for any InputSymbol which maps to this instance.
  u8 visibility = 0;
};

struct ResolveError {
  enum class Code : u8 {
    MultipleDefinition, // Two strong definitions.
    TlsMismatch,        // A symbol is defined as TLS in one file and not in another.
  };
  // A short, lower-case description, such as "multiple definition".
  static std::string_view to_string(Code code);

  Code code;
  // Output symbol associated with the conflict.
  SymbolID symbol;
  InputSymbolID original, conflicting;
  bool operator==(const ResolveError &) const = default;
};

// Merge all InputSymbols into a single set of all non-local symbols usable for the linker. Where multiple definitions
// conflict, the highest ranked definition wins. Symbol versions are recorded so that foo@@V defines the default version
// of foo, so it becomes the canonical value foor foo and foo@V. Otherwise, foo@V is a different symbol from foo.
class ResolvedSymbols {
public:
  // IDs are [0, size()), with 0 as STN_UNDEF.
  u32 size() const noexcept { return static_cast<u32>(_names.size()); }
  // The symbol entry id refers to, after following any forwards. Throws std::out_of_range if no entry has this ID.
  SymbolID of(InputSymbolID id) const;
  // The name of the matching symbol entry, or throws std::out_of_range if no entry has this ID.
  std::string_view name(SymbolID id) const;
  // Throws std::out_of_range if no symbol has this ID.
  const Symbol &symbol(SymbolID id) const;
  // Conflicts found while resolving, in input order. The link must fail if this is non-empty.
  std::span<const ResolveError> errors() const noexcept { return _errors; }

private:
  friend ResolvedSymbols resolve_symbols(const InputSymbols &table);
  // Indexed by InputSymbolID.
  std::vector<SymbolID> _of;
  // Both are indexed by SymbolID. Entry 0 is STN_UNDEF.
  std::vector<std::string_view> _names = std::vector<std::string_view>(1);
  std::vector<Symbol> _symbols = std::vector<Symbol>(1);
  // Indexed by SymbolID. If the symbol collapsed into another (e.g., foo@V into foo), this points to the best ranking
  // symbol matching the same name. Otherwise points to 0 / STN_UNDEF.
  std::vector<SymbolID> _forward;
  // Errors generated during resolve_symbols.
  std::vector<ResolveError> _errors;
};

// Decide which symbol of the link each entry of table refers to, and which definition owns each symbol. Follows Ian
// Lance Taylor's rules for symbol resolution as mold implements them (get_rank and merge_visibility in mold's
// input-files.cc): a strong definition beats a weak one, which beats a common one; between equals, the earliest in
// input order wins.
// TODO: merge common sizes
// TODO: separate ranking for archive members and shared libraries
ResolvedSymbols resolve_symbols(const InputSymbols &table);
} // namespace pepp::link
