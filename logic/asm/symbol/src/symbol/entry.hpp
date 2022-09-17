#pragma once

// File: entry.hpp
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

#include <climits>
#include <memory>
#include <string>

#include "types.hpp"
#include "value.hpp"

namespace symbol {

template <typename value_t> class LeafTable;

// Currently unused, will eventuallly be used to track trace tag information alongside a symbol.
// TODO: Determine how to track debugging information.
struct format {
    SymbolReprFormat format;
    uint32_t size = 0;
};

/*!
 * \brief A symbol represents a named value or location in memory that may be defined at most once.
 *
 * Symbols provided by this namespace are meant to be flexibile--supporting microprogrammin and ELF usage.
 *
 * Symbols that are undefined after linkage should cause an error; symbols defined multiple times should cause
 * immediate errors.
 * Some examples of symbol values are: currently undefined (symbol::empty), an address of a line of code
 * (symbol::value_location), or a numeric constant (symbol::value_const)
 *
 * As this class exposes all of its data publicly, it is up to the user to ensure that properties such as binding
 * and definition state are updated correctly.
 * This API decision was made because it is impossible for a symbol to know if its value is being set because it is
 * defined, or if the value is being set to handle relocation. Simply put, the symbol doesn't know enough to update
 * these fields.
 *
 * \tparam value_t An unsigned integral type that is large enough to contain the largest address on the target system.
 */
template <typename value_t> class entry {

  public:
    // Default constructor, assumes value is symbol::value_empty
    entry(typename symbol::LeafTable<value_t> &parent, std::string name);
    ~entry() = default;

    //! Non-owning reference to containing symbol table.
    typename symbol::LeafTable<value_t> const &parent;

    //! Unique name as appearing in source code.
    std::string name;
    //! Keep track of how many times this symbol's name has been defined.
    definition_state state;
    /*! The binding type of this symbol (i.e., global vs local).
     * \sa symbol::binding*/
    binding_t binding;
    /*! The value taken on by this symbol.
     * \sa symbol::abstract_value */
    std::shared_ptr<symbol::abstract_value<value_t>> value;
};

} // end namespace symbol

#include "entry.tpp"