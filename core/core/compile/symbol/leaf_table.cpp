/*
 * Copyright (c) 2023-2026 J. Stanley Warford, Matthew McRaven
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

#include "core/compile/symbol/leaf_table.hpp"
#include <sstream>
#include "core/compile/symbol/entry.hpp"
#include "core/compile/symbol/types.hpp"
#include "core/compile/symbol/value.hpp"
#include "fmt/format.h"

pepp::core::symbol::LeafTable::LeafTable(u16 pointer_size) noexcept
    : _pointer_size(pointer_size), _pool(std::make_shared<bts::StringPool>()),
      _entries(0, bts::PooledString::Hash(_pool.get()), bts::PooledString::Equals(_pool.get())) {}

pepp::core::symbol::LeafTable::LeafTable(u16 pointer_size, std::shared_ptr<bts::StringPool> pool) noexcept
    : _pointer_size(pointer_size), _pool(std::make_shared<bts::StringPool>()),
      _entries(0, bts::PooledString::Hash(_pool.get()), bts::PooledString::Equals(_pool.get())) {}

std::optional<pepp::core::symbol::LeafTable::entry_ptr_t> pepp::core::symbol::LeafTable::import(LeafTable &other,
                                                                                                std::string_view name) {
  auto extSym = other.get(name);
  if (extSym == std::nullopt) return std::nullopt;
  auto intSym = this->define(name);
  intSym->binding = symbol::Binding::Weak;

  auto value = std::make_shared<symbol::ConstantValue>(extSym.value()->value->value());
  intSym->value = value;
  return intSym;
}

pepp::core::symbol::LeafTable::entry_ptr_t pepp::core::symbol::LeafTable::reference(std::string_view name) noexcept {
  // Create a new entry if one does not already exist
  auto pooled = _pool->insert(name, bts::StringPool::AddNullTerminator::Never);
  if (auto it = _entries.find(pooled); it == _entries.end()) {
    auto sv = _pool->find(pooled).value();
    return _entries[pooled] = std::make_shared<symbol::Entry>(*this, sv);
  } else return it->second;
}

pepp::core::symbol::LeafTable::entry_ptr_t pepp::core::symbol::LeafTable::define(std::string_view name) noexcept {
  auto entry = reference(name);

  if (entry->is_undefined()) entry->state = DefinitionState::Single;
  else if (entry->is_singly_defined()) {
    if (entry->binding == symbol::Binding::Weak) entry->state = DefinitionState::ExternalMultiple;
    else entry->state = DefinitionState::Multiple;
  }
  return entry;
}

std::optional<pepp::core::symbol::LeafTable::entry_ptr_t>
pepp::core::symbol::LeafTable::get(std::string_view name) const noexcept {
  if (auto pooled = _pool->find(name); !pooled) return std::nullopt;
  else if (auto item = _entries.find(*pooled); item != _entries.end()) return item->second;
  else return std::nullopt;
}

bool pepp::core::symbol::LeafTable::exists(std::string_view name) const noexcept {
  if (auto pooled = _pool->find(name); !pooled) return false;
  else return _entries.find(*pooled) != _entries.cend();
}

auto pepp::core::symbol::LeafTable::entries() const noexcept -> const pepp::core::symbol::LeafTable::map_t & {
  return _entries;
}

auto pepp::core::symbol::LeafTable::entries() noexcept -> pepp::core::symbol::LeafTable::map_t & { return _entries; }

u16 pepp::core::symbol::LeafTable::pointer_size() const noexcept { return _pointer_size; }

void pepp::core::symbol::increment_offset(LeafTable &table, u64 offset, u64 threshold) noexcept {
  for (auto const &[_, entry] : table.entries()) {
    if (auto loc = std::dynamic_pointer_cast<symbol::LocationValue>(entry->value); loc && loc->base() >= threshold)
      loc->increment_offset(offset);
  }
}

void pepp::core::symbol::set_offset(LeafTable &table, u64 offset, u64 threshold) noexcept {
  for (auto const &[_, entry] : table.entries()) {
    if (auto loc = std::dynamic_pointer_cast<symbol::LocationValue>(entry->value); loc && loc->base() >= threshold)
      loc->set_offset(offset);
  }
}

void pepp::core::symbol::enumerate(const LeafTable &table, std::vector<std::shared_ptr<Entry>> &out) {
  for (const auto &[_, entry] : table.entries()) out.push_back(entry);
}

std::string pepp::core::symbol::table_listing(const LeafTable &table, u8 max_bytes) {
  // Helper lambda to pretty print a single symbol.
  static constexpr auto format = [](const auto &sym, u8 max_bytes) {
    return fmt::format("{: <9} 0x{:0{}X}", sym->name, sym->value->value()(), 2 * max_bytes);
  };

  std::vector<std::shared_ptr<Entry>> symbols;
  enumerate(table, symbols);

  bool lhs = true;
  std::ostringstream ss;

  for (auto it = symbols.cbegin(); it != symbols.cend(); it++, lhs ^= true) {
    if (lhs) ss << format(*it, max_bytes);
    else ss << "         " << format(*it, max_bytes) << std::endl;
  }

  // The last entry was on the left, but we did not append a newline. Explicit insert newline to fix #399
  if (!lhs) ss << std::endl;
  return ss.str();
}
