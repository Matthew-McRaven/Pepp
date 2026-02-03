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
#pragma once

#include <memory>
#include <unordered_map>
#include "core/ds/string_pool.hpp"
#include "core/integers.h"
namespace pepp::bts {
class StringPool;
}
namespace pepp::core::symbol {
class Entry;
/*
 * Some implementation ideas drawn from:
 * Design and Implementation of the Symbol Table for Object-Oriented Programming
 * Language, Yangsun Lee 2017, http://dx.doi.org/10.14257/ijdta.2017.10.7.03
 */

// This version returns to a branch-and-leaf design.
// For languages without scope, a single symbol table is the correct implementation pattern.
// For languages with scope, an alternative type will be provided.
// Symbols that have been marked as deleted will have their entries remain in the table, but with a DeletedValue
// assigned.

class LeafTable : public std::enable_shared_from_this<LeafTable> {
public:
  using entry_ptr_t = std::shared_ptr<symbol::Entry>;
  using map_t = std::unordered_map<bts::PooledString, entry_ptr_t, bts::PooledString::Hash, bts::PooledString::Equals>;

  // Create a StringPool exclusively for this LeafTable.
  explicit LeafTable(u16 pointer_size) noexcept;
  // Share a StringPool with other users. That StringPool must be append-only!
  LeafTable(u16 pointer_size, std::shared_ptr<bts::StringPool> pool) noexcept;
  ~LeafTable() noexcept = default;

  LeafTable(const LeafTable &) = delete;
  LeafTable &operator=(const LeafTable &) = delete;
  LeafTable(LeafTable &&) noexcept = delete;
  LeafTable &operator=(LeafTable &&) noexcept = delete;

  // Copy a named symbol from another table into this one as a constant.
  // Returns a defined symbol in this table, or nullopt if not found in other.
  std::optional<entry_ptr_t> import(symbol::LeafTable &other, std::string_view name);

  // Either returns an existing symbol entry or creates a new, undefined one.
  entry_ptr_t reference(std::string_view name) noexcept;
  // If name not already defined, creates a new, singly-defined symbol entry.
  // Otherwise, modifies the existing symbol to be multiply-defined.
  entry_ptr_t define(std::string_view name) noexcept;
  // Unlike reference, will return nullopt if symbol not found.
  std::optional<entry_ptr_t> get(std::string_view name) const noexcept;
  // Returns true if this table contains the matching symbol.
  bool exists(std::string_view name) const noexcept;
  // Number of bytes needed to store a pointer to another symbol
  u16 pointer_size() const noexcept;

  // Return all symbols contained by the table.
  auto entries() const noexcept -> map_t const &;
  // Return all symbols contained by the table. Mutable to allow transformations by visitors.
  auto entries() noexcept -> map_t &;

private:
  u16 _pointer_size;

  // Symbols are often repeated between multiple tables, use a shared
  std::shared_ptr<bts::StringPool> _pool;
  bts::PooledString::Hash _hash;
  bts::PooledString::Equals _comparator;

  map_t _entries;
};

// For each symbol in the table, whose "base" is >= threshold, increment its "offset".
void increment_offset(LeafTable &table, u64 offset, u64 threshold = 0) noexcept;
void set_offset(LeafTable &table, u64 offset, u64 threshold = 0) noexcept;
// Create a vector of all symbols within this table.
void enumerate(LeafTable const &table, std::vector<std::shared_ptr<Entry>> &out);
// Create a string representation of a symbol table
std::string table_listing(LeafTable const &table, u8 max_bytes);

} // namespace pepp::core::symbol
