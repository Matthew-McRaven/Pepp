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

#include "core/link/symbol_resolution.hpp"
#include <algorithm>
#include <stdexcept>
#include <unordered_map>
#include "core/link/symbol_versions.hpp"

std::string_view pepp::link::GatherError::to_string(Code code) {
  switch (code) {
  case Code::BadStringTable: return "symbol table's string table does not exist";
  case Code::BadFirstGlobal: return "symbol table's sh_info is out-of-bounds";
  case Code::LocalAfterFirstGlobal: return "local symbol after sh_info";
  case Code::GlobalBeforeFirstGlobal: return "non-local symbol before sh_info";
  case Code::LocalOnlyType: return "non-local file or section symbol";
  }
  return "unknown error";
}

std::string_view pepp::link::InputSymbols::name(InputSymbolID id) const {
  if (id.value >= _names.size()) throw std::out_of_range("InputSymbols: no symbol with this ID");
  return _names[id.value];
}

pepp::link::SymbolAttributes pepp::link::InputSymbols::attributes(InputSymbolID id) const {
  if (id.value >= _attributes.size()) throw std::out_of_range("InputSymbols: no symbol with this ID");
  return _attributes[id.value];
}

std::optional<pepp::link::InputSymbolID> pepp::link::InputSymbols::find(bts::ElfFileID id, u32 index) const {
  if (!id || id.value >= _non_locals.size()) return std::nullopt;
  else if (index == 0) return InputSymbolID(0);
  // ELF puts a file's locals before its non-locals, which start at sh_info, so the non-locals are one contiguous span.
  const auto &range = _non_locals[id.value];
  if (index < range.first_global || index - range.first_global >= range.count) return std::nullopt;
  return range.first_id + (index - range.first_global);
}

pepp::bts::ElfFileID pepp::link::InputSymbols::file_of(InputSymbolID id) const {
  if (!id || id.value >= _names.size()) throw std::out_of_range("InputSymbols: no symbol with this ID");
  // Files' first IDs ascend. A file without non-locals shares its first ID with the next file, so take the last range
  // starting at or before id, which is the one holding it.
  const auto starts_after = [](InputSymbolID target, const NonLocalRange &range) { return target < range.first_id; };
  const auto it = std::upper_bound(_non_locals.begin() + 1, _non_locals.end(), id, starts_after);
  return bts::ElfFileID(static_cast<u32>(it - _non_locals.begin() - 1));
}

namespace {
using pepp::link::InputSymbolID;
using pepp::link::SymbolID;

// Match the tiers as described by mold (get_rank in input-files.cc). Gaps are preserved so I can add the missing tiers
// in the future.
enum class Tier : u64 {
  Strong = 1,
  Weak = 2,
  // 3 and 4 are strong and weak definitions in a DSO or an unreachable archive member
  Common = 5,
  // 6 is a common symbol in an unreachable archive member: not ranked yet.
  Undefined = 7,
};

// The rank of a symbol table entry. A lower value has higher priority, copied from mold. FileID's underlying value
// preserves command-line order and is used to break ties between entries of the same tier.
u64 rank_of(Tier tier, pepp::bts::ElfFileID file) { return (bits::to_underlying(tier) << 32) | file.value; }
u64 rank_of(pepp::link::SymbolAttributes attributes, pepp::bts::ElfFileID file) {
  using enum Tier;
  const auto tier = attributes.undefined ? Undefined : attributes.common ? Common : attributes.weak ? Weak : Strong;
  return rank_of(tier, file);
}
Tier tier_of(u64 rank) { return static_cast<Tier>(rank >> 32); }

// Provide a ranking of how constrainining the visibility value is. Higher values are more constraining, and the highest
// value is selected. Unlike mold, internal is not combined with hidden.
u8 constraint_of(u8 visibility) {
  using enum pepp::bts::SymbolVisibility;
  switch (static_cast<pepp::bts::SymbolVisibility>(visibility)) {
  case STV_INTERNAL: return 3;
  case STV_HIDDEN: return 2;
  case STV_PROTECTED: return 1;
  default: return 0;
  }
}

// What resolution knows about one symbol.
struct Candidate {
  // The best definition so far, and its rank. Every symbol starts undefined, the worst tier so the first definition
  // will always become the owner.
  InputSymbolID owner;
  u64 rank = rank_of(Tier::Undefined, pepp::bts::ElfFileID{});
  // The first entry with a type, and whether it is thread-local. Type-mismatches will lead to link errors.
  InputSymbolID typed;
  bool tls = false;
  // The most constraining visibility ever seen for this name.
  u8 visibility = 0;
};

Candidate candidate_of(const pepp::link::InputSymbols &table, InputSymbolID id) {
  const auto attributes = table.attributes(id);
  Candidate ret{.tls = bool(attributes.tls), .visibility = attributes.visibility};
  if (!attributes.untyped) ret.typed = id;
  if (!attributes.undefined) ret.owner = id, ret.rank = rank_of(attributes, table.file_of(id));
  return ret;
}

// If from is a better candidate than into, update into with the fields of from.
void merge(Candidate &into, const Candidate &from, SymbolID symbol, std::vector<pepp::link::ResolveError> &errors) {
  using enum pepp::link::ResolveError::Code;
  // Keep the more restrictive visibility for the output.
  if (constraint_of(from.visibility) > constraint_of(into.visibility)) into.visibility = from.visibility;
  // Declarations must agree on whether the symbol is thread-local. STT_NOTYPE can be ignored.
  if (from.typed && !into.typed) into.typed = from.typed, into.tls = from.tls;
  else if (from.typed && from.tls != into.tls)
    errors.push_back({TlsMismatch, symbol, std::min(into.typed, from.typed), std::max(into.typed, from.typed)});
  if (!from.owner) return; // Must be undefined. No point in checking attribute.s Hope someone else defines it later.
  // Two strong definitions in object files are a multiple definition error.
  if (tier_of(from.rank) == Tier::Strong && tier_of(into.rank) == Tier::Strong)
    errors.push_back({MultipleDefinition, symbol, std::min(into.owner, from.owner), std::max(into.owner, from.owner)});
  if (from.rank < into.rank) into.owner = from.owner, into.rank = from.rank;
}
} // namespace

pepp::link::ResolvedSymbols pepp::link::resolve_symbols(const InputSymbols &table) {
  using Kind = VersionedName::Kind;
  ResolvedSymbols ret;
  ret._of.reserve(table.size());
  // First index reserved for STN_UNDEF.
  ret._of.emplace_back(0);
  // Auxiliary storage for each symbol allocated in _of.
  std::vector<Candidate> candidates(1);
  // De-duplicate all symbols which share the same name
  std::unordered_map<std::string_view, SymbolID> ids;
  // Every defined default version, and every `foo@V` symbol, which may yet turn out to be foo's default.
  DefaultVersionSet default_versions;
  // Symbols which need to be revisited because they were explicitly version.
  // A later default version may make any of these equal to the default, in which case we need _forward entries.
  std::vector<SymbolID> versioned;

  // Perform a single pass over the input symbols, selecting the best ranking input symbol for each name.
  for (u32 it = 1; it < table.size(); ++it) {
    const InputSymbolID id(it);
    const auto name = table.name(id);
    const auto split_name = split(name);
    // Extract the name/version parts of the symbol entry. Record default version for later fixups.
    const auto key = split_name.kind == Kind::Default ? split_name.base : name;
    if (split_name.kind == Kind::Default && !table.undefined(id)) default_versions.insert(split_name);

    // If not already present, must extend the various vectors to hold a new SymbolID
    const auto [entry, inserted] = ids.try_emplace(key, SymbolID(static_cast<u32>(ret._names.size())));
    if (inserted) {
      ret._names.push_back(key);
      candidates.emplace_back();
      if (split_name.kind == Kind::NonDefault) versioned.push_back(entry->second);
    }
    // Establishing mapping from input symbol to an already allocated output symbol (entry)
    const auto symbol_id = entry->second;
    ret._of.push_back(symbol_id);
    merge(candidates[symbol_id.value], candidate_of(table, id), symbol_id, ret._errors);
  }

  // Perform a second, shorter pass over the versioned symbols.
  // Each sym@V whose V turned out to be the default due to a later definition should be collapsed to sym.
  for (const auto from : versioned) {
    // Check if there is a default version definition for this symbol. If not, forwarding is impossible.
    const auto split_name = split(ret._names[from.value]);
    if (!default_versions.contains(split_name)) continue;
    const auto into = ids.at(split_name.base);
    // Lazily allocate the forwarding vector. It is unlikely to be used in most links.
    if (ret._forward.empty()) ret._forward.resize(ret._names.size());
    ret._forward[from.value] = into;
    merge(candidates[into.value], candidates[from.value], into, ret._errors);
  }

  // candidates already holds STN_UNDEF's entry. Copy the merged candidates into our symbol vector.
  ret._symbols.clear();
  ret._symbols.reserve(candidates.size());
  for (const auto &candidate : candidates) ret._symbols.push_back({candidate.owner, candidate.visibility});
  // Sort errors by the offending symbols input ID.
  std::ranges::stable_sort(ret._errors, {}, &ResolveError::conflicting);
  return ret;
}

std::string_view pepp::link::ResolveError::to_string(Code code) {
  switch (code) {
  case Code::MultipleDefinition: return "multiple definition";
  case Code::TlsMismatch: return "thread-local and non-thread-local symbols share a name";
  }
  return "unknown error";
}

pepp::link::SymbolID pepp::link::ResolvedSymbols::of(InputSymbolID id) const {
  if (id.value >= _of.size()) throw std::out_of_range("ResolvedSymbols: no entry with this ID");
  const auto symbol = _of[id.value];
  return _forward.empty() || !_forward[symbol.value] ? symbol : _forward[symbol.value];
}

std::string_view pepp::link::ResolvedSymbols::name(SymbolID id) const {
  if (id.value >= _names.size()) throw std::out_of_range("ResolvedSymbols: no symbol with this ID");
  return _names[id.value];
}

const pepp::link::Symbol &pepp::link::ResolvedSymbols::symbol(SymbolID id) const {
  if (id.value >= _symbols.size()) throw std::out_of_range("ResolvedSymbols: no symbol with this ID");
  return _symbols[id.value];
}
