/*
 * Copyright (c) 2023 J. Stanley Warford, Matthew McRaven
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

#include <QtCore>
#include "types.hpp"
#include "value.hpp"

namespace symbol {

class Table;

// Currently unused, will eventually be used to track trace tag information
// alongside a symbol.
// TODO: Determine how to track debugging information.
struct format {
  SymbolReprFormat format;
  quint32 size = 0;
};

/*!
 * \brief A symbol represents a named value or location in memory that may be
 * defined at most once.
 *
 * Symbols provided by this namespace are meant to be flexibile--supporting
 * microprogramming and ELF usage.
 *
 * Symbols that are undefined after linkage should cause an error; symbols
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
  Entry(symbol::Table &parent, QString name);
  ~Entry() = default;

  //! Non-owning reference to containing symbol table.
  typename symbol::Table &parent;

  //! Unique name as appearing in source code.
  QString name;
  //! Keep track of how many times this symbol's name has been defined.
  DefinitionState state;
  /*! The binding type of this symbol (i.e., global vs local).
   * \sa symbol::Binding*/
  Binding binding;
  /*! The value taken on by this symbol.
   * \sa symbol::value ::Abstract*/
  QSharedPointer<symbol::value::Abstract> value;

  /*! The section in respect to which this symbol is defined.
   * Setting this field require knowledge of hte final layout of the elf file,
   * therefore, this value is set late in the assembly process
   */
  // Elf32 uses 16bit, ELF64 use 32bit, so just pick the largest of the two
  // types.
  quint32 section_index = 0;

  bool is_singly_defined() const;
  bool is_undefined() const;
};
} // end namespace symbol
