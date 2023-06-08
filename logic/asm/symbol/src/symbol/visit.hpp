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

// File: visit.hpp
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
#include <QSharedPointer>

#include "symbol_globals.hpp"
#include "types.hpp"

namespace symbol {
class Table;
class Entry;
/*!
 * \brief Determine immediate parent of table
 * \arg table A node in a hierarchical symbol table.
 * \returns Returns the immediate parent of table if it exists, else returns
 * table
 */
SYMBOL_EXPORT QSharedPointer<symbol::Table>
parent(QSharedPointer<symbol::Table> table);
SYMBOL_EXPORT QSharedPointer<const symbol::Table>
parent(QSharedPointer<const symbol::Table> table);

/*!
 * \brief Returns the top-level node in a hierarchical symbol table.
 *
 */
SYMBOL_EXPORT QSharedPointer<symbol::Table>
rootTable(QSharedPointer<symbol::Table> table);
SYMBOL_EXPORT QSharedPointer<const symbol::Table>
rootTable(QSharedPointer<const symbol::Table> table);

SYMBOL_EXPORT QList<QSharedPointer<symbol::Entry>>
selectByName(QSharedPointer<symbol::Table> table, const QString &name,
             TraversalPolicy policy = TraversalPolicy::kChildren);
/*!
 * \brief Determine if any table in the hierarchical symbol table contains a
 * symbol with a particular name.
 * \arg table A node in a hierarchical symbol table.
 * \arg name The name of the symbol to be found.
 * \returns Returns true if at least one child of table contains
 */
SYMBOL_EXPORT bool exists(QSharedPointer<symbol::Table> table,
                          const QString &name,
                          TraversalPolicy policy = TraversalPolicy::kChildren);

/*!
 * \brief For each symbol in table, if the value is a value_location, adjust the
 * offset field by "offset" if the base field >= threshold.
 * \arg table A node in a hierarchical symbol table.
 * \arg offset value that Location's offset will be set to
 * \arg threshold value above with a Location's base must be for the offset to
 * be applied.
 */
SYMBOL_EXPORT void
adjustOffset(QSharedPointer<symbol::Table> table, quint64 offset,
             quint64 threshold = 0,
             TraversalPolicy policy = TraversalPolicy::kChildren);

/*!
 * \brief Create a list of all symbols in the symbol table
 * \arg table A node in a hierarchical symbol table.
 */
SYMBOL_EXPORT QList<QSharedPointer<symbol::Entry>>
enumerate(QSharedPointer<symbol::Table> table,
          TraversalPolicy policy = TraversalPolicy::kChildren);

/*!
 * \brief Create a string representation of a symbol table
 * \arg table A node in a hierarchical symbol table.
 */
SYMBOL_EXPORT QString
tableListing(QSharedPointer<symbol::Table> table, quint8 maxBytes,
             TraversalPolicy policy = TraversalPolicy::kChildren);
} // end namespace symbol
