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
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>
#include "core/ds/opaque_handle.hpp"
#include "core/formats/elf/packed_access_symbol.hpp"
#include "core/formats/elf/packed_input_group.hpp"
#include "core/link/diagnostics.hpp"

namespace pepp::link {

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
  // Including STN_UNDEF/0 IDs are [0, size()).
  u32 size() const noexcept { return static_cast<u32>(_names.size()); }
  // All throw std::out_of_range if id >= size(). Name points into the ELF file's string table storage.
  std::string_view name(InputSymbolID id) const;
  bool undefined(InputSymbolID id) const { return attributes(id).undefined; }
  SymbolAttributes attributes(InputSymbolID id) const;
  // Throws std::out_of_range unless id is a common definition.
  CommonSize common(InputSymbolID id) const;
  // The ID of symbol index of file id: STN_UNDEF for index 0, or nullopt if the symbol is local or does not exist.
  std::optional<InputSymbolID> find(bts::ElfFileID id, u32 index) const;
  // The file that symbol id belongs to. Throws std::out_of_range for STN_UNDEF, which belongs to every file, or if
  // id >= size().
  bts::ElfFileID file_of(InputSymbolID id) const;
  // The non-local symbols of a file. Throws std::out_of_range if no file has this ID.
  std::pair<InputSymbolID, InputSymbolID> ids_of(bts::ElfFileID id) const;
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
  // Sparse since few symbols are common.
  std::unordered_map<InputSymbolID, CommonSize, pepp::handle_hash<InputSymbolID>> _commons;

  template <bts::ElfBits B, bts::ElfEndian E>
  friend std::optional<InputSymbols> gather_symbols(Diagnostics &diagnostics,
                                                    const bts::PackedInputElfGroup<B, E> &group);
};

// Extract all non-local symbols for each ELF file in the group. Files without symbol tables are skipped. Archive's
// symbol index is ignored, meaning members' symbol tables are re-read from their ELF symtabs.
// A malformed symbol table is Fatal, citing the offending symbol index, or 0 for the table as a whole.
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

// Identifies one symbol of the link after mergin InputSymbols. 0 remains STN_UNDEF
using SymbolID = pepp::OpaqueHandle<struct SymbolTag, u32>;

// Resolved definition of a single symbol.
struct Symbol {
  // The strongest definition. Refer to the owner's attributes for information other than visibility.
  InputSymbolID owner;
  // Record the most constraining visibility for any InputSymbol which maps to this instance.
  u8 visibility = 0;
};

// Limit a file's participation in resolve_symbols.
enum class Liveness : u8 {
  // Not part of the link. Its entries map to STN_UNDEF.
  Excluded,
  // An archive member not yet included. Its definitions rank below every live file's.
  Lazy,
  // Always included in the link. Lazy files referenced by live files become live.
  Live,
};

// Merge all InputSymbols into a single set of all non-local symbols usable for the linker. Where multiple definitions
// conflict, the highest ranked definition wins. Symbol versions are recorded so that foo@@V defines the default version
// of foo, so it becomes the canonical value foor foo and foo@V. Otherwise, foo@V is a different symbol from foo.
class ResolvedSymbols {
public:
  // IDs are [0, size()), with 0 as STN_UNDEF.
  u32 size() const noexcept { return static_cast<u32>(_names.size()); }
  // The symbol entry id refers to, after following any forwards. STN_UNDEF for an entry in an Excluded file. Throws
  // std::out_of_range if no entry has this ID.
  SymbolID of(InputSymbolID id) const;
  // The name of the matching symbol entry, or throws std::out_of_range if no entry has this ID.
  std::string_view name(SymbolID id) const;
  // Throws std::out_of_range if no symbol has this ID.
  const Symbol &symbol(SymbolID id) const;
  // The merged size and alignment of id if its owner is a common definition, or nullopt.
  std::optional<CommonSize> common(SymbolID id) const;
  // The best ranked foo@@V definition that a foo@V or foo entry forwards to. STN_UNDEF if not forwarded.
  InputSymbolID provider(InputSymbolID id) const;

private:
  friend ResolvedSymbols resolve_symbols(Diagnostics &diagnostics, const InputSymbols &table,
                                         const std::vector<Liveness> &files);
  // Indexed by InputSymbolID.
  std::vector<SymbolID> _of;
  // Both are indexed by SymbolID. Entry 0 is STN_UNDEF.
  std::vector<std::string_view> _names = std::vector<std::string_view>(1);
  std::vector<Symbol> _symbols = std::vector<Symbol>(1);
  // If the symbol collapsed into another (e.g., foo@V into foo), record both the collapsed symbol and the original
  // owner.
  struct Forward {
    // Points to the best ranking symbol matching the same name.
    SymbolID into;
    // The original symbol definition before collapse.
    InputSymbolID provider;
  };
  // Indexed by SymbolID.
  std::vector<Forward> _forward;
  // Sparse since few symbols are common.
  std::unordered_map<SymbolID, CommonSize, pepp::handle_hash<SymbolID>> _commons;
};

// Decide which symbol of the link each entry of table refers to, and which definition owns each symbol. Follows Ian
// Lance Taylor's rules for symbol resolution as mold implements them (get_rank and merge_visibility in mold's
// input-files.cc): a strong definition beats a weak one, which beats a common one; between equals, the earliest in
// input order wins.
// Drivers must call this twice. On the first pass, we determine which Lazy archive members should be promoted to live
// (while still gathering all Lazy members' symbols). The second pass should occur after mark_live_objects performed
// that promotion. Cannot be one pass, otherwise lazy members' undefineds could cause a link that should pass to fail.
// The alternative is iterating in resolve_symbols until we reach a fixed-point, promoting on-the-fly.
// files is indexed by file ID and must have table.files() + 1 entries, or this throws std::invalid_argument.
// Conflicts and strong undefined references are added as Errors in input order.
// TODO: merge common sizes
// TODO: shared libraries
ResolvedSymbols resolve_symbols(Diagnostics &diagnostics, const InputSymbols &table,
                                const std::vector<Liveness> &files);

// For each file, convert Lazy to Live if ResolvedSymbols contains a strong reference to that file.
// Otherwise, demote Lazy to Excluded. Live and excluded files are unchanged. files is indexed by file ID and must
// have table.files() + 1 entries, or this throws std::invalid_argument. This is equivalent to mold's mark_live_objects
// (passes.cc, input-files.cc)
void mark_live_objects(const InputSymbols &table, const ResolvedSymbols &symbols, std::vector<Liveness> &files);
} // namespace pepp::link
