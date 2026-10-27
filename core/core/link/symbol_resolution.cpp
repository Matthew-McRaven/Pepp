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

std::pair<pepp::link::InputSymbolID, pepp::link::InputSymbolID>
pepp::link::InputSymbols::ids_of(bts::ElfFileID id) const {
  if (!id || id.value >= _non_locals.size()) throw std::out_of_range("InputSymbols: no file with this ID");
  const auto &range = _non_locals[id.value];
  return {range.first_id, range.first_id + range.count};
}

namespace {
using pepp::link::InputSymbolID;
using pepp::link::Liveness;
using pepp::link::SymbolID;

// Match the tiers as described by mold (get_rank in input-files.cc). Gaps are preserved so I can add the missing tiers
// in the future.
enum class Tier : u64 {
  Strong = 1,
  Weak = 2,
  // Definitions in a Lazy archive member. mold also ranks a DSO's definitions as 3 and 4.
  StrongArchive = 3,
  WeakArchive = 4,
  Common = 5,
  CommonArchive = 6,
  Undefined = 7,
};

// The rank of a symbol table entry. A lower value has higher priority, copied from mold. FileID's underlying value
// preserves command-line order and is used to break ties between entries of the same tier.
u64 rank_of(Tier tier, pepp::bts::ElfFileID file) { return (bits::to_underlying(tier) << 32) | file.value; }
u64 rank_of(pepp::link::SymbolAttributes attributes, pepp::bts::ElfFileID file, bool live) {
  using enum Tier;
  if (attributes.undefined) return rank_of(Undefined, file);
  else if (attributes.common) return rank_of(live ? Common : CommonArchive, file);
  else if (attributes.weak) return rank_of(live ? Weak : WeakArchive, file);
  return rank_of(live ? Strong : StrongArchive, file);
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

// What ranking needs to know about a symbol beyond its Symbol, kept only while resolving.
struct Ranking {
  // The owner's rank. Every symbol starts undefined, the worst tier so the first definition will always become the
  // owner.
  u64 rank = rank_of(Tier::Undefined, pepp::bts::ElfFileID{});
  // The first entry with a type, and whether it is thread-local. Type-mismatches will lead to link errors.
  InputSymbolID typed;
  bool tls = false;
};

// One entry's claim on its symbol.
struct Candidate {
  pepp::link::Symbol symbol;
  Ranking ranking;
};

Candidate candidate_of(const pepp::link::InputSymbols &table, InputSymbolID id, pepp::bts::ElfFileID file,
                       bool live) {
  const auto attributes = table.attributes(id);
  Candidate ret{.symbol = {.visibility = attributes.visibility}, .ranking = {.tls = bool(attributes.tls)}};
  if (!attributes.untyped) ret.ranking.typed = id;
  if (!attributes.undefined) ret.symbol.owner = id, ret.ranking.rank = rank_of(attributes, file, live);
  return ret;
}

// If from is a better candidate than into, update into and its ranking with the fields of from.
void merge(pepp::link::Symbol &into, Ranking &ranking, const Candidate &from, SymbolID symbol,
           std::vector<pepp::link::ResolveError> &errors) {
  using enum pepp::link::ResolveError::Code;
  // Keep the more restrictive visibility for the output.
  if (constraint_of(from.symbol.visibility) > constraint_of(into.visibility)) into.visibility = from.symbol.visibility;
  // Declarations must agree on whether the symbol is thread-local. STT_NOTYPE can be ignored.
  const auto typed = from.ranking.typed;
  if (typed && !ranking.typed) ranking.typed = typed, ranking.tls = from.ranking.tls;
  else if (typed && from.ranking.tls != ranking.tls)
    errors.push_back({TlsMismatch, symbol, std::min(ranking.typed, typed), std::max(ranking.typed, typed)});
  const auto owner = from.symbol.owner;
  if (!owner) return; // Must be undefined. No point in checking attributes. Hope someone else defines it later.
  // Two strong definitions in object files are a multiple definition error.
  if (tier_of(from.ranking.rank) == Tier::Strong && tier_of(ranking.rank) == Tier::Strong)
    errors.push_back({MultipleDefinition, symbol, std::min(into.owner, owner), std::max(into.owner, owner)});
  if (from.ranking.rank < ranking.rank) into.owner = owner, ranking.rank = from.ranking.rank;
}

// Each canonical key's SymbolID, as mold's symbol_map, plus what default-version forwarding needs.
struct SymbolMap {
  // De-duplicate all symbols which share the same name
  std::unordered_map<std::string_view, SymbolID> ids;
  // Every defined default version.
  pepp::link::DefaultVersionSet default_versions;
  // Symbols which need to be revisited because they were explicitly versioned (foo@V).
  // A later default version may make any of these equal to the default, in which case we need _forward entries.
  std::vector<SymbolID> versioned;
};

// Map an input symbol to an output SymbolID, substituting for default versions where possible, i.e., foo@@V is foo.
// Like mold's get_symbol (input-files.cc), a new key is added to the map, and appended to names.
SymbolID get_symbol(SymbolMap &map, std::vector<std::string_view> &names, const pepp::link::InputSymbols &table,
                    InputSymbolID id) {
  using Kind = pepp::link::VersionedName::Kind;
  const auto name = table.name(id);
  const auto split_name = pepp::link::split(name);
  // Extract the name/version parts of the symbol entry. Record default version for later fixups.
  const auto key = split_name.kind == Kind::Default ? split_name.base : name;
  if (split_name.kind == Kind::Default && !table.undefined(id)) map.default_versions.insert(split_name);
  // If not already present, must extend the various vectors to hold a new SymbolID
  const auto [entry, inserted] = map.ids.try_emplace(key, SymbolID(static_cast<u32>(names.size())));
  if (inserted) {
    names.push_back(key);
    if (split_name.kind == Kind::NonDefault) map.versioned.push_back(entry->second);
  }
  return entry->second;
}

// The symbol that versioned symbol name (sym@V) collapses into if V turned out to be sym's default version due to a
// later definition, or STN_UNDEF.
SymbolID forward_of(const SymbolMap &map, std::string_view name) {
  // Check if there is a default version definition for this symbol. If not, forwarding is impossible.
  const auto split_name = pepp::link::split(name);
  return map.default_versions.contains(split_name) ? map.ids.at(split_name.base) : SymbolID(0);
}

void check_file_count(const pepp::link::InputSymbols &table, const std::vector<Liveness> &files) {
  if (files.size() != table.files() + 1) throw std::invalid_argument("Need one entry per file, plus index 0");
}
} // namespace

pepp::link::ResolvedSymbols pepp::link::resolve_symbols(const InputSymbols &table,
                                                        const std::vector<Liveness> &files) {
  check_file_count(table, files);
  ResolvedSymbols ret;
  ret._of.reserve(table.size());
  // First index reserved for STN_UNDEF.
  ret._of.emplace_back(0);
  SymbolMap map;
  // Indexed by SymbolID, beside ret._symbols.
  std::vector<Ranking> rankings(1);
  std::vector<ResolveError> errors;
  // Strong undefined references, which are errors if nothing defines their symbol.
  std::vector<InputSymbolID> references;

  // Perform a single pass over the input symbols, selecting the best ranking input symbol for each name.
  // Iterate over files first so we can avoid a lg(n) reverse-lookup of the file from the symbol on each iteration.
  for (u32 it = 1; it <= table.files(); ++it) {
    const bts::ElfFileID file(it);
    const auto [first, last] = table.ids_of(file);
    // Even if the file is excluded, we want our global index over symbols to be stable.
    // So, allocate STN_UNDEF entries equal to the number of non-locals in the file.
    if (files[it] == Liveness::Excluded) {
      ret._of.insert(ret._of.end(), last.value - first.value, SymbolID(0));
      continue;
    }
    const bool live = files[it] == Liveness::Live;
    for (auto id = first; id < last; ++id) {
      const auto symbol = get_symbol(map, ret._names, table, id);
      // Establishing mapping from input symbol to an already allocated output symbol (entry)
      ret._of.push_back(symbol);
      // A new symbol starts undefined.
      ret._symbols.resize(ret._names.size());
      rankings.resize(ret._names.size());
      // Pick the best input symbol for each output symbol.
      merge(ret._symbols[symbol.value], rankings[symbol.value], candidate_of(table, id, file, live), symbol, errors);
      const auto attributes = table.attributes(id);
      if (attributes.undefined && !attributes.weak) references.push_back(id);
    }
  }

  // Perform a second, shorter pass over the versioned symbols.
  // Each sym@V whose V turned out to be the default due to a later definition should be collapsed to sym.
  for (const auto from : map.versioned) {
    const auto into = forward_of(map, ret._names[from.value]);
    if (!into) continue;
    // Lazily allocate the forwarding vector. It is unlikely to be used in most links.
    if (ret._forward.empty()) ret._forward.resize(ret._names.size());
    ret._forward[from.value] = into;
    const Candidate candidate{ret._symbols[from.value], rankings[from.value]};
    merge(ret._symbols[into.value], rankings[into.value], candidate, into, errors);
  }

  // Perform a single pass to catch all undefined references from the source programs.
  // All undefined references in live files are a linker error. Errors are produced per input file which references
  // that symbol rather than per-symbol in the output (following mold).
  for (const auto id : references)
    if (const auto symbol = ret.of(id); !ret._symbols[symbol.value].owner)
      errors.push_back({ResolveError::Code::Undefined, symbol, InputSymbolID(0), id});
  // Sort errors by the offending symbols input ID.
  std::ranges::stable_sort(errors, {}, &ResolveError::conflicting);
  ret._errors = std::move(errors);
  return ret;
}

void pepp::link::mark_live_objects(const InputSymbols &table, const ResolvedSymbols &symbols,
                                   std::vector<Liveness> &files) {
  check_file_count(table, files);
  // A worklist of all files that are live
  std::vector<bts::ElfFileID> worklist;
  for (u32 file = 1; file <= table.files(); ++file)
    if (files[file] == Liveness::Live) worklist.emplace_back(file);

  // Search all files reachable from our initial roots
  while (!worklist.empty()) {
    const auto file = worklist.back();
    worklist.pop_back();
    // Search all the symbol references of that file.
    const auto [first, last] = table.ids_of(file);
    for (auto id = first; id < last; ++id) {
      const auto owner = symbols.symbol(symbols.of(id)).owner;
      if (!owner) continue;
      const auto attributes = table.attributes(id);
      const bool undefined_ref = attributes.undefined && !attributes.weak;
      const bool common_ref = attributes.common && !table.attributes(owner).common;
      const auto owner_file = table.file_of(owner);
      // If the referenced symbol is from a lazy file, promote that file to live, and add it to the worklist.
      if ((undefined_ref || common_ref) && files[owner_file.value] == Liveness::Lazy)
        files[owner_file.value] = Liveness::Live, worklist.push_back(owner_file);
    }
  }
}

std::string_view pepp::link::ResolveError::to_string(Code code) {
  switch (code) {
  case Code::MultipleDefinition: return "multiple definition";
  case Code::TlsMismatch: return "thread-local and non-thread-local symbols share a name";
  case Code::Undefined: return "undefined symbol";
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
