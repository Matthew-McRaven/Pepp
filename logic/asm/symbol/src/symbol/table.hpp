#pragma once

// File: table.hpp
/*
    The Pep/10 suite of applications (Pep10, Pep10CPU, Pep10Term) are
    simulators for the Pep/10 virtual machine, and allow users to
    create, simulate, and debug across various levels of abstraction.

    Copyright (C) 2021 J. Stanley Warford & Matthew McRaven, Pepperdine University

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

#include <boost/range/adaptors.hpp>
#include <boost/range/any_range.hpp>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <variant>

#include "entry.hpp"

namespace symbol {

template<typename value_t> class BranchTable;
template<typename value_t> class LeafTable;

/*
 * Some implementation ideas drawn from:
 * Design and Implementation of the Symbol Table for Object-Oriented Programming Language,
 * Yangsun Lee 2017, http://dx.doi.org/10.14257/ijdta.2017.10.7.03
 */

/*!
 * \brief Contains a pointer to either a symbol table or a table of pointers to tables.
 *
 * Since some tables have values and some tables are a list of other tables, I need a way to describe both.
 * These tables form a tree, which makes a visitor pattern a natural way of interacting with the whole tree.
 * I wanted to take advantage of std::visit instead of building my own visitor pattern.
 * For this reason, I'm required to use a variant rather than inheritance.
 * This ends up making NodeType pervasive throughout any code touching symbol tables, warranting a shorthand definition.
 *
 * \tparam value_t An unsigned integral type that is large enough to contain the largest address on the target system.
 */
template<typename value_t>
using NodeType =
    std::variant<std::shared_ptr<symbol::BranchTable<value_t>>, std::shared_ptr<symbol::LeafTable<value_t>>>;

/*!
 * \brief A symbol table which does not contain any symbols, but instead contains other symbol tables.
 *
 * This table forms the inner node of a hierarchical symbol table.
 * It contains no symbols of its own, to separate the functionality of containing symbols from containing tables.
 * Like the LeafTable, it is add only.
 * No symbols or tables are allowed to be deleted to prevent confusion when dealing with externals' definitions
 * changing.
 *
 * \tparam value_t An unsigned integral type that is large enough to contain the largest address on the target system.
 */
template<typename value_t> class BranchTable : public std::enable_shared_from_this<BranchTable<value_t>> {
public:
  //! Default constructor only used for Global BranchTable
  BranchTable() = default;
  [[maybe_unused]] explicit BranchTable(std::shared_ptr<BranchTable<value_t>> parent);
  ~BranchTable() = default;

  //	Copying and move OK
  BranchTable( const BranchTable& ) = default;
  BranchTable& operator =( const BranchTable& ) = default;
  BranchTable( BranchTable&& ) noexcept = default;
  BranchTable& operator=( BranchTable&& ) noexcept = default;

  /*!
   * \brief Register an existing symbol table as a child of this table.
   * Instead of calling this directly, it is usually best to use insert_leaf or insert_branch.
   * These methods make sure that the added table has the proper parent, and the parent has an owning reference to the
   * child. \arg child A symbol table that is \sa symbol::insert_leaf \sa symbol::insert_branch
   */
  void add_child(NodeType<value_t> child);
  /*!
   * \brief Fetch the list of all children under this table.
   * \returns A mutable list of all children under this table.
   * Please don't abuse the fact that children are non-const.
   */
  std::list<NodeType<value_t>> children() { return children_; }

  //! A pointer to the parent of this table. If the pointer is null, this table is the root of the tree.
  const std::weak_ptr<BranchTable<value_t>> parent_ = {};
private:
  std::list<NodeType<value_t>> children_;
};

template<typename value_t> class LeafTable : public std::enable_shared_from_this<LeafTable<value_t>> {
public:
  using entry_ptr_t = std::shared_ptr<symbol::entry<value_t>>;
  using range = boost::any_range<entry_ptr_t, boost::forward_traversal_tag, entry_ptr_t &, std::ptrdiff_t>;
  using const_range =
      boost::any_range<const entry_ptr_t, boost::forward_traversal_tag, const entry_ptr_t &, std::ptrdiff_t>;

  LeafTable() = default;
  explicit LeafTable(std::shared_ptr<BranchTable<value_t>> parent);
  ~LeafTable() = default;

  //	Copying and move OK
  LeafTable( const LeafTable& ) = default;
  LeafTable& operator =( const LeafTable& ) = default;
  LeafTable( LeafTable&& ) noexcept = default;
  LeafTable& operator=( LeafTable&& ) noexcept = default;

  /*!
   * \brief Unlike reference, get() will not create an entry in the table if the symbol fails
   * to be found.
   * \returns Pointer to found symbol or nullopt if not found.
   */
  std::optional<entry_ptr_t> get(const std::string &name) const;

  /*!
   * \brief Create a symbol entry. Do data validations checks to see if
   * symbol is already declared globally.
   * \returns Pointer to symbol.
   */
  entry_ptr_t reference(const std::string &name);

  /*!
   * \brief May a symbol entry. Do data validations checks to see if
   * symbol is already declared globally. Sets state of variable
   * \returns Pointer to symbol.
   */
  entry_ptr_t define(const std::string &name);

  /*!
   * Once a symbol has been marked as global, there is no un-global'ing it.
   * This function handles walking the tree and pointing other local symbols to this tables global instance.
   */
  void mark_global(const std::string &name);
  //! Returns true if this table (not checking any other table in the hierarchy) contains a symbol with the matching
  //! name.
  bool exists(const std::string &name) const;

  //! Return all symbols contained by the table.
  auto entries() const -> const_range;
  //! Return all symbols contained by the table. Mutable to allow transformations by visitors.
  auto entries() -> range;
  //! A pointer to the parent of this table. If the pointer is null, this table is the root of the tree.
  const std::weak_ptr<BranchTable<value_t>> parent_ = {};

private:
  std::map<std::string, entry_ptr_t> name_to_entry_;
};

//! Wrap the creation of a new BranchTable and the correct setup of the parent/child relationship.
template<typename value_t>
std::shared_ptr<LeafTable<value_t>> insert_leaf(std::shared_ptr<BranchTable<value_t>> parent);

//! Wrap the creation of a new LeafTable and the correct setup of the parent/child relationship.
template<typename value_t>
std::shared_ptr<BranchTable<value_t>> insert_branch(std::shared_ptr<BranchTable<value_t>> parent);

//! Return the parent of table if it exists, otherwise return table.
template<typename value_t> NodeType<value_t> parent(NodeType<value_t> table);

} // end namespace symbol
#include "table.tpp"