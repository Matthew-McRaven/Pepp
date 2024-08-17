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

#include <QEnableSharedFromThis>
#include <QSharedPointer>
#include <QString>
#include <iostream>
#include <optional>
#include "entry.hpp"
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
class Table : public QEnableSharedFromThis<Table> {
public:
  using entry_ptr_t = QSharedPointer<symbol::Entry>;
  using map_t = QMap<QString, entry_ptr_t>;
  using range = decltype(std::declval<map_t &>().asKeyValueRange());
  using const_range = detail::asConstKeyValueRange<const map_t>;

  // See: https://stackoverflow.com/a/54127343
  struct child_const_iterator {
    using iter_t = typename QList<QSharedPointer<symbol::Table>>::const_iterator;
    iter_t it;
    child_const_iterator(iter_t init);
    child_const_iterator &operator++();
    const QSharedPointer<const symbol::Table> operator*() const;
    bool operator!=(const child_const_iterator &rhs) const;
  };
  using child_iterator = QList<QSharedPointer<symbol::Table>>::iterator;
  //! Default constructor only used for top level Tables
  explicit Table(quint16 pointerSize);
  [[maybe_unused]] explicit Table(QSharedPointer<Table> parent);
  ~Table() = default;

  //  Copying and move OK
  Table(const Table &) = default;
  Table &operator=(const Table &) = delete;
  Table(Table &&) noexcept = default;
  Table &operator=(Table &&) noexcept = delete;

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
  child_iterator begin();
  child_iterator end();
  child_const_iterator cbegin() const;
  child_const_iterator cend() const;

  /*!
   * \brief Create a symbol locally that points to a symbol in an external
   * table. Handles setting various symbol flags correctly for import.
   * \returns a define()'ed symbol if name is in other, else nullopt if not
   * found.
   */
  std::optional<entry_ptr_t> import(symbol::Table &other, const QString &name);

  /*!
   * \brief Unlike reference, get() will not create an entry in the table if
   * the symbol fails to be found. \returns Pointer to found symbol or
   * nullopt if not found.
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

  //! Number of bytes needed to store a pointer to another symbol
  quint16 pointerSize() const;

private:
  quint16 _pointerSize;
  QList<QSharedPointer<Table>> _children;
  map_t _name_to_entry;
};
} // end namespace symbol
