#pragma once

// File: visit.hpp
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

#include <iostream>
#include <list>
#include <map>
#include <string>
#include <variant>

#include <boost/range/adaptors.hpp>
#include <boost/range/any_range.hpp>

#include "entry.hpp"
#include "table.hpp"

namespace symbol {

//! Find the root of the symbol tree.
template <typename value_t> struct RootVisitor {
    NodeType<value_t> operator()(std::shared_ptr<BranchTable<value_t>> table);
    NodeType<value_t> operator()(std::shared_ptr<LeafTable<value_t>> table);
};

//! Gather all symbols sharing the same name into a list.
template <typename value_t> class SelectByNameVisitor {
  public:
    SelectByNameVisitor(std::string name);
    SelectByNameVisitor() = delete;
    ~SelectByNameVisitor() = default;

    //	Copying and move OK
    SelectByNameVisitor( const SelectByNameVisitor& ) = default;
    SelectByNameVisitor& operator =( const SelectByNameVisitor& ) = default;
    SelectByNameVisitor( SelectByNameVisitor&& ) noexcept = default;
    SelectByNameVisitor& operator=( SelectByNameVisitor&& ) noexcept = default;

    std::list<std::shared_ptr<symbol::entry<value_t>>> operator()(std::shared_ptr<BranchTable<value_t>> table);
    std::list<std::shared_ptr<symbol::entry<value_t>>> operator()(std::shared_ptr<LeafTable<value_t>> table);

  private:
    std::string target;
};

//! Check for the existence of a symbol by name
template <typename value_t> class ExistenceVisitor {
  public:
    ExistenceVisitor(std::string name);
    ExistenceVisitor() = delete;
    ~ExistenceVisitor() = default;

    //	Copying and move OK
    ExistenceVisitor( const ExistenceVisitor& ) = default;
    ExistenceVisitor& operator =( const ExistenceVisitor& ) = default;
    ExistenceVisitor( ExistenceVisitor&& ) noexcept = default;
    ExistenceVisitor& operator=( ExistenceVisitor&& ) noexcept = default;

    bool operator()(std::shared_ptr<BranchTable<value_t>> table);
    bool operator()(std::shared_ptr<LeafTable<value_t>> table);

  private:
    std::string target;
};

//! Modify the value of all symbol::value_locations. Useful for relocation.
template <typename value_t> class AdjustOffsetVisitor {
  public:
    AdjustOffsetVisitor(value_t offset);
    AdjustOffsetVisitor(value_t offset, value_t threshold);
    AdjustOffsetVisitor() = delete;
    ~AdjustOffsetVisitor() = default;

    //	Copying and move OK
    AdjustOffsetVisitor( const AdjustOffsetVisitor& ) = default;
    AdjustOffsetVisitor& operator =( const AdjustOffsetVisitor& ) = default;
    AdjustOffsetVisitor( AdjustOffsetVisitor&& ) noexcept = default;
    AdjustOffsetVisitor& operator=( AdjustOffsetVisitor&& ) noexcept = default;

    void operator()(std::shared_ptr<BranchTable<value_t>> table);
    void operator()(std::shared_ptr<LeafTable<value_t>> table);

  private:
    value_t offset_ = {0}, threshhold_ = {0};
};

//! List all symbols in a tree
template <typename value_t> struct EnumerationVisitor {
    std::list<std::shared_ptr<symbol::entry<value_t>>> operator()(std::shared_ptr<BranchTable<value_t>> table);
    std::list<std::shared_ptr<symbol::entry<value_t>>> operator()(std::shared_ptr<LeafTable<value_t>> table);
};


/*
 * Helper methods that wrap visitor creation and std::visit invocation.
 * Must also update binding enum!!
 */
enum class TraversalPolicy {
    kChildren = 0,  /*!< Only visit the current node and its children.*/
    kSiblings = 1,  /*!< Visit the current node, its children, its siblings, and its siblings children.*/
    kWholeTree = 2, /*!< Visit every node in the tree.*/
};

/*!
 * \brief Find the root table in a hierarchy given any of its descendants or itself.
 * \arg table A node in a hierarchical symbol table.
 * \returns Returns the input table if that table has no parent, or it's greatest ancestor in the parent exists.
 * \tparam value_t An unsigned integral type that is large enough to contain the largest address on the target system.
 * \sa symbol::RootVisitor
 */
template <typename value_t> symbol::NodeType<value_t> root_table(NodeType<value_t> table);

template <typename value_t>
std::list<std::shared_ptr<symbol::entry<value_t>>> select_by_name(NodeType<value_t> table, const std::string &name,
                                                                  TraversalPolicy policy = TraversalPolicy::kChildren);

/*!
 * \brief Determine if any table in the hierarchical symbol table contains a symbol with a particular name.
 * \arg table A node in a hierarchical symbol table.
 * \arg name The name of the symbol to be found.
 * \returns Returns true if at least one child of table contains
 * \tparam value_t An unsigned integral type that is large enough to contain the largest address on the target system.
 * \sa symbol::ExistenceVisitor
 */
template <typename value_t>
bool exists(NodeType<value_t> table, const std::string &name, TraversalPolicy policy = TraversalPolicy::kChildren);

/*!
 * \brief For each symbol in table, if the value is a value_location, adjust the offset field by "offset" if the base
 * field >= threshold. 
 * \arg table A node in a hierarchical symbol table. 
 * \arg offset 
 * \arg threshold 
 * \tparam value_t Anunsigned integral type that is large enough to contain the largest address on the target system. 
 * \sa symbol::AdjustOffsetVisitor
 */
template <typename value_t>
void adjust_offset(NodeType<value_t> table, value_t offset, value_t threshhold = 0,
                   TraversalPolicy policy = TraversalPolicy::kChildren);

/*!
 * \brief Create a list of all symbols in the symbol table
 * \arg table A node in a hierarchical symbol table.
 * \tparam value_t An unsigned integral type that is large enough to contain the largest address on the target system. 
 * \sa symbol::EnumerationVisitor
 */
template <typename value_t>
std::list<std::shared_ptr<symbol::entry<value_t>>> enumerate_symbols(NodeType<value_t> table, TraversalPolicy policy = TraversalPolicy::kChildren);

/*!
 * \brief Create a string representation of a symbol table
 * \arg table A node in a hierarchical symbol table.
 * \tparam value_t An unsigned integral type that is large enough to contain the largest address on the target system. 
 * \sa symbol::EnumerationVisitor
 */
template <typename value_t>
std::string symbol_table_listing(NodeType<value_t> table, TraversalPolicy policy = TraversalPolicy::kChildren);
} // end namespace symbol

#include "visit.tpp"