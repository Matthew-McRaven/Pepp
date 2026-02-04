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
#include <string_view>
#include "core/compile/symbol/types.hpp"

namespace pepp::core::symbol {
class LeafTable;
class AbstractValue;

/*!
 * \brief A symbol represents a named value or location in memory that may be
 * defined at most once.
 *
 * Symbols provided by this namespace are meant to be flexibile--supporting
 * microprogramming and ELF usage.
 *
 * Symbols that are undefined should cause an error during linkage and loading; symbols
 * defined multiple times should cause immediate errors. Some examples of symbol
 * values are: currently undefined (symbol::empty), an address of a line of code
 * (symbol::value_location), or a numeric constant (symbol::value_const)
 *
 * As this class exposes all of its data publicly, it is up to the user to
 * ensure that properties such as binding and definition state are updated
 * correctly. This API decision was made because it is impossible for a symbol
 * to know if its value is being set because it is defined, or if the value is
 * being set to handle relocation. Simply put, the symbol doesn't know enough to
 * update these fields.
 *
 * Values have 64 bits, with a mandatory bitmask and length in the value.
 */
class Entry {
public:
  // Default constructor, assumes value is symbol::value_empty
  Entry(symbol::LeafTable &parent, std::string_view name) noexcept;
  ~Entry() = default;

  //! Keep track of how many times this symbol's name has been defined.
  DefinitionState state;
  // The binding type of this symbol (i.e., global vs local).
  Binding binding;
  //! Unique name as appearing in source code.
  std::string_view name;
  //! Non-owning reference to containing symbol table.
  typename symbol::LeafTable &parent;
  // The value taken on by this symbol.
  std::shared_ptr<symbol::AbstractValue> value;

  bool is_singly_defined() const noexcept;
  bool is_undefined() const noexcept;
};
} // namespace pepp::core::symbol
