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
#include "core/link/symbol_inputs.hpp"

namespace pepp::link {

// A symbol of the link after merging InputSymbols. 0 remains STN_UNDEF.
using SymbolID = pepp::OpaqueHandle<struct SymbolTag, u32>;

// Resolved definition of a single symbol.
struct Symbol {
  // The strongest definition. Refer to the owner's attributes for information other than visibility.
  InputSymbolID owner;
  // The most constraining visibility of every InputSymbol that maps to this symbol.
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
  // A file whose symbol definitions are eligible for merging (as SHN_ABS) but whose sections will not appear in the
  // final output. It is treated as a live file. Undefined references are ignored.
  JustSymbols,
};

// The non-local symbols of the link, merged from InputSymbols. Where definitions conflict, the highest ranked wins.
// foo@@V defines the default version of foo, so foo and foo@V both resolve to it. Without a default, foo@V is a
// different symbol from foo.
class ResolvedSymbols {
public:
  // IDs are [0, size()), with 0 as STN_UNDEF.
  u32 size() const noexcept { return static_cast<u32>(_names.size()); }
  // Returns the symbol of entry id, after following any forwards. Returns STN_UNDEF for an entry in an Excluded file.
  // Throws std::out_of_range if no entry has this ID.
  SymbolID of(InputSymbolID id) const;
  // Returns the name of symbol id. Throws std::out_of_range if no symbol has this ID.
  std::string_view name(SymbolID id) const;
  // Throws std::out_of_range if no symbol has this ID.
  const Symbol &symbol(SymbolID id) const;
  // Returns the merged size and alignment of id if its owner is a common definition. Otherwise, returns nullopt.
  std::optional<CommonSize> common(SymbolID id) const;
  // Returns the symbol with this name after following any forward, or STN_UNDEF if there is none.
  SymbolID find(std::string_view name) const;
  // Returns the best ranked foo@@V definition for a forwarding foo@V or foo entry, or STN_UNDEF if id does not forward.
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
