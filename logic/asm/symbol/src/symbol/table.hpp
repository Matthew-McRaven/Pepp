#pragma once

// File: table.hpp
/*
    The Pep/10 suite of applications (Pep10, Pep10CPU, Pep10Term) are
    simulators for the Pep/10 virtual machine, and allow users to
    create, simulate, and debug across various levels of abstraction.

    Copyright (C) 2021 J. Stanley Warford & Matthew McRaven, Pepperdine
   University

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <QEnableSharedFromThis>
#include <QSharedPointer>
#include <QString>
#include <iostream>
#include <optional>

#include "entry.hpp"
#include "symbol_globals.hpp"
// #include "visit.hpp"

namespace symbol {

namespace detail {
template <typename T> class asConstKeyValueRange {
public:
  asConstKeyValueRange(T &data) : m_data{data} {}

  auto begin() { return m_data.constKeyValueBegin(); }

  auto end() { return m_data.constKeyValueEnd(); }

private:
  T &m_data;
};
} // namespace detail
/*
 * Some implementation ideas drawn from:
 * Design and Implementation of the Symbol Table for Object-Oriented Programming
 * Language, Yangsun Lee 2017, http://dx.doi.org/10.14257/ijdta.2017.10.7.03
 */

/*!
 * \brief A symbol table which does not contain any symbols, but instead
 * contains other symbol tables.
 *
 * Unlike the previous branch+leaf design, this table contains symbols and has
 * child tables. Symbols are not allowed to be removed, but they are allowed to
 * be marked as deleted.
 */
class SYMBOL_EXPORT Table : public QEnableSharedFromThis<Table> {
public:
  using entry_ptr_t = QSharedPointer<symbol::Entry>;
  using map_t = QMap<QString, entry_ptr_t>;
  using range = decltype(std::declval<map_t &>().asKeyValueRange());
  using const_range = detail::asConstKeyValueRange<const map_t>;
  //! Default constructor only used for top level Tables
  Table() = default;
  [[maybe_unused]] explicit Table(QSharedPointer<Table> parent);
  ~Table() = default;

  //	Copying and move OK
  Table(const Table &) = default;
  Table &operator=(const Table &) = default;
  Table(Table &&) noexcept = default;
  Table &operator=(Table &&) noexcept = default;

  /*!
   * \brief Create and register a new symbol table as a child of this table
   */
  QSharedPointer<Table> addChild();
  /*!
   * \brief Fetch the list of all children under this table.
   * \returns A mutable list of all children under this table.
   * Please don't abuse the fact that children are non-const.
   */
  QList<QSharedPointer<Table>> children();

  /*!
   * \brief Unlike reference, get() will not create an entry in the table if the
   * symbol fails to be found.
   * \returns Pointer to found symbol or nullopt if not found.
   */
  std::optional<entry_ptr_t> get(const QString &name) const;

  /*!
   * \brief Create a symbol entry if it doesn't already exist. Do data
   * validations checks to see if symbol is already declared globally.
   * \returns
   * Pointer to symbol.
   */
  entry_ptr_t reference(const QString &name);

  /*!
   * \brief Create a symbol entry if it doesn't already exist. Do data
   * validations checks to see if symbol is already declared globally. Sets
   * definition state of variable.
   * \returns Pointer to symbol.
   */
  entry_ptr_t define(const QString &name);

  /*!
   * Once a symbol has been marked as global, there is no un-global'ing it.
   * This function handles walking the tree and pointing other local symbols to
   * this tables global instance.
   */
  void markGlobal(const QString &name);
  //! Returns true if this table (not checking any other table in the hierarchy)
  //! contains a symbol with the matching name.
  bool exists(const QString &name) const;

  //! Return all symbols contained by the table.
  auto entries() const -> const_range;
  //! Return all symbols contained by the table. Mutable to allow
  //! transformations by visitors.
  auto entries() -> range;

  //! A pointer to the parent of this table. If the pointer is null, this table
  //! is the root of the tree.
  const QWeakPointer<Table> parent;

private:
  QList<QSharedPointer<Table>> _children;
  map_t _name_to_entry;
};
} // end namespace symbol
