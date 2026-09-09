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
#include <string>
#include "core/compile/symbol/entry.hpp"
#include "core/ds/hash/djb.hpp"
#include "core/compile/symbol/types.hpp"
#include "core/compile/symbol/value.hpp"
#include "core/formats/elf/managed_elf.hpp"
#include "core/formats/elf/managed_section_gnu_hash.hpp"
#include "core/formats/elf/managed_section_strtab.hpp"
#include "core/formats/elf/packed_types.hpp"
#include "core/math/bitmanip/log2.hpp"

pepp::bts::ManagedSymbolTable::ManagedSymbolTable(std::shared_ptr<core::symbol::LeafTable> symbols)
    : _symbols(std::move(symbols)) {
  if (!_symbols) throw std::logic_error("ManagedSymbolTable: needs a symbol table to describe");
}

void pepp::bts::ManagedSymbolTable::set_section(const entry_ptr_t &entry, SectionRef ref) {
  if (!entry) throw std::logic_error("ManagedSymbolTable: cannot place a null symbol in a section");
  _sections[entry] = ref;
}

pepp::bts::ManagedSymbolTable::entry_ptr_t pepp::bts::ManagedSymbolTable::section_symbol(SectionRef section) {
  using namespace core::symbol;
  if (auto found = _section_symbols.find(section); found != _section_symbols.end()) return found->second;
  // Section symbols are nameless, which causes problems for the LeafTable. So we have to store them here.
  // These symbols must be local.
  auto entry = std::make_shared<Entry>(std::string_view{});
  entry->state = DefinitionState::Single;
  entry->binding = Binding::Local;
  entry->value = std::make_shared<SectionValue>();
  set_section(entry, section);
  return _section_symbols[section] = entry;
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

namespace {
// Convert our hash policy into the set of actual parameters for use by .gnu.hash.
// symndx is the index of the first symbol to be hashed, while cnt is the total number of symbols to be hashed.
pepp::bts::GnuHashParameters resolve_params(pepp::bts::GnuHashPolicy policy, u32 symndx, u32 cnt,
                                            pepp::bts::ElfBits bits) {
  const u32 word_bits = word_bytes(bits) * 8u;
  pepp::bts::GnuHashParameters params;
  const u32 words = bits::ceil_div(cnt * policy.bloom_bits_per_symbol, word_bits);
  params.nbuckets = std::max<u32>(1, cnt / std::max<u32>(1, policy.symbols_per_bucket));
  params.symndx = symndx, params.hashed_count = cnt;
  // Power-of-two words. Avoid taking lg(0), which will throw
  params.maskwords = 1u << bits::ceil_log2(std::max<u32>(1, words));
  params.shift2 = bits::ceil_log2(word_bits);
  return params;
}
} // namespace

void pepp::bts::ManagedSymbolTable::freeze(const std::function<bool(const entry_ptr_t &)> &keep, ElfBits bits) {
  using namespace core::symbol;
  const auto &entries = _symbols->entries();
  auto &ordered = _frozen.emplace();
  // Over-allocates when keep garbage collects symbols, but prevents allocations during the insert step.
  ordered.reserve(entries.size() + _section_symbols.size() + 1);
  ordered.push_back(nullptr); // Meet ELF requirement that null symbol is first.
  const auto consider = [&](const entry_ptr_t &entry) {
    if (!entry || is_tombstone(entry)) return;
    else if (keep && !keep(entry)) return;
    else ordered.push_back(entry);
  };
  for (const auto &[_, entry] : entries) consider(entry);
  // Must evaluate section symbols for garbage collection.
  for (const auto &[_, entry] : _section_symbols) consider(entry);

  // Do not sort the null symbol. Ensure all locals precede any non-local.
  std::sort(ordered.begin() + 1, ordered.end(), [this](const entry_ptr_t &lhs, const entry_ptr_t &rhs) {
    const bool left_local = lhs->binding == Binding::Local, right_local = rhs->binding == Binding::Local;
    if (left_local != right_local) return left_local;
    // Section symbols are all nameless, so the section each stands for is what separates them.
    else if (lhs->name != rhs->name) return lhs->name < rhs->name;
    return section_of(lhs) < section_of(rhs);
  });

  // If a hash policy is set, determine the parameters and sort non-local symbols by hash % nbuckets.
  _hash_parameters.reset();
  if (!_hash_policy) return;
  // All locals must come before any non-local, and hash requires symbols be sorted by
  // hash % nbuckets. Therefore hash can only cover non-locals.
  const u32 symndx = first_nonlocal(), hashed_cnt = static_cast<u32>(ordered.size()) - symndx;
  _hash_parameters = resolve_params(*_hash_policy, symndx, hashed_cnt, bits);
  // .gnu.hash groups together symbols by bucket to make search easier. Within a bucket, names break ties to keep the
  // result reproducible. Because we already sorted by hash % nbuckets, the writer will not need to swap any symbols.
  const auto nbuckets = _hash_parameters->nbuckets;
  std::sort(ordered.begin() + symndx, ordered.end(), [nbuckets](const entry_ptr_t &lhs, const entry_ptr_t &rhs) {
    const auto left = djb(lhs->name) % nbuckets, right = djb(rhs->name) % nbuckets;
    if (left != right) return left < right;
    return lhs->name < rhs->name;
  });
}

std::optional<pepp::bts::GnuHashParameters> pepp::bts::ManagedSymbolTable::hash_parameters() const {
  if (!is_frozen()) throw std::logic_error("ManagedSymbolTable: symbol order was never frozen");
  return _hash_parameters;
}

void pepp::bts::freeze_symbols(ManagedElf &elf, SectionRef ref,
                               const std::function<bool(const ManagedSymbolTable::entry_ptr_t &)> &keep) {
  auto *sec = elf.section(ref);
  if (!sec) throw std::logic_error("freeze_symbols: no such section");
  auto *table = sec->content_as<ManagedSymbolTable>();
  if (!table) throw std::logic_error("freeze_symbols: this section holds no symbol table");
  table->freeze(keep, elf.bits());
  sec->info = u32{table->first_nonlocal()};
  // GNU `as` likes to align this section to platform word size so that you can mmap and cast easily.
  sec->addralign = word_bytes(elf.bits());

  // Create a string table for this symbol table if one does not already exist.
  if (!sec->link) {
    const bool dynamic = sec->type == SectionTypes::SHT_DYNSYM;
    sec->link = elf.add_section(dynamic ? ".dynstr" : ".strtab", SectionTypes::SHT_STRTAB);
  }

  const auto params = table->hash_parameters();
  if (!params) return;
  // Search for an existing .gnu.hash which points to this symbol table
  for (auto it = ManagedElf::SHN_UNDEF; it <= elf.last_section(); ++it) {
    auto *candidate = elf.section(it);
    auto *existing = candidate ? candidate->content_as<ManagedGnuHash>() : nullptr;
    if (existing && existing->symtab() == ref) return existing->set_parameters(*params);
  }
  // Otherwise create a new .gnu.hash
  auto *hash = elf.section(elf.add_section(".gnu.hash", SectionTypes::SHT_GNU_HASH));
  hash->link = ref;
  hash->addralign = word_bytes(elf.bits());
  hash->make_content<ManagedGnuHash>(ref, sec->link, *params);
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

void pepp::bts::build_strtabs_for_symtabs(ManagedElf &elf, std::span<const SectionRef> live) {
  for (auto ref : live) {
    auto *sec = elf.section(ref);
    if (!sec) continue;
    auto *symbols = sec->content_as<ManagedSymbolTable>();
    if (!symbols) continue;

    auto *linked = elf.section(sec->link);
    if (!linked) throw std::logic_error("build_strtabs_for_symtabs: a symbol table links to no section");
    else if (linked->type != SectionTypes::SHT_STRTAB)
      throw std::logic_error("build_strtabs_for_symtabs: a symbol table's sh_link is not a string table");

    // Ensure ManagedStringTable exists. Do not clear it if pointing to an existing table
    auto *names = linked->content_as<ManagedStringTable>();
    if (!names) names = &linked->make_content<ManagedStringTable>();

    // Neither the null symbol nor section symbols have names.
    for (const auto &entry : symbols->frozen_order())
      if (entry && !entry->name.empty()) names->insert(entry->name);
  }
}
