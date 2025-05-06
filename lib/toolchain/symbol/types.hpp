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
#include <QDebug>
#include <QVariant>
#include <QtCore>

/*!
 * \file types.hpp
 * \namespace symbol
 */

/*!
 * \brief Code that handles creation and manipulation of ELF-style symbol
 * tables.
 *
 * */
namespace symbol {
Q_NAMESPACE;
// Format for symbols
enum class SymbolReprFormat : qint32 { kNone, kChar, kDec, kHex };

/*!
 * The definition_state of a symbol tracks the number of times a symbol has been
 * defined within a translation unit.
 *
 * Multiply defined symbols should cause assembly or linkage to fail very
 * quickly, as there is no way to recove Very often, this property is modified
 * via symbol::table<T>::define(...) for a particular symbol .
 *
 * \sa symbol::Table::define
 */
enum class DefinitionState {
  kUndefined,        //!< A symbol is not defined, and referenced 1+ times.
  kSingle,           //!< Singlely Defined: A symbol is defined once, and referenced 0+
                     //!< times.
  kMultiple,         //!< Multiply Defined: A symbol is defined 2+ times, and referenced
                     //!< 0+ times.
  kExternalMultiple, //!< Defined in this translation unit and exported from
                     //!< another translation unit.
};
Q_ENUM_NS(DefinitionState);

/*!
 * A symbol's type determines how the value field is associated with the
 * symbol's definition.
 *
 * These fields are inspired by the ELF specification, with some extensions to
 * allow for ease-of-use with our assembler toolchain. As demonstrated in
 * `/src/masm/elf/pack.hpp` there is more-or-less a 1-1 mapping between our
 * types and ELF types.
 */
enum class Type {
  /*! Represent a not-yet-defined symbol's value.
   * Maps to ELF's STT_NOTYPE.*/
  kEmpty,
  /*! The associated symbol is an address in memory of data.
   * These types arise from symbol declarations like: \code{.s}hi:.WORD
   * 2\endcode Maps to ELF's STT_OBJECT. */
  kObject,
  /*! The associated symbol represents an address in memory of code.
   * These types arise from symbol declarations like: \code{.s}hi:NOP\endcode
   * Maps to ELF's STT_FUNC. */
  kCode,
  /*! The associated symbol represents an numeric value or string in memory.
   * These types arise from symbol declarations like: \code{.asm}hi:.EQUATE
   * 10\endcode Maps to ELF's STT_ABS.*/
  kConstant,
  /*! The associated symbol's value is that of another symbol.
   * It was mainly used to migrate IO ports from the operating system to user
   * programs.
   * \deprecated With the advent of the new ELF linker, symbols should
   * not migrate between symbol tables. Does not map to any ELF symbol type, as
   * this value is deprecated.*/
  kPtrToSym,
  /*! The associated symbol has been marked for deletion.
   * Does not map to any ELF symbol type, as this symbol no longer exists.*/
  kDeleted,
};
Q_ENUM_NS(Type)

/*!
 * A symbol's binding determines if it is visible outside the current
 * translation unit.
 *
 * These binding types are drawn directly from the ELF specification, as these
 * fields are meant to ease translation from our custom IR to the ELF format.
 * Please see: https://refspecs.linuxfoundation.org/elf/elf.pdf.
 *
 * A symbol declared as global in one translation unit may not be declared as
 * global again in another translation unit. Additioanlly, if symbol defined in
 * one translation unit as local and another as global, then a linker must raise
 * a "multiple definition" error.
 *
 * If the same symbol is defined in both translation units as local it will link
 * just fine. Local symbols aren't exported across translation units.
 *
 * Weak symbols act like local symbols and provide a definiton / value within a
 * translation unit. However, if a global definition for the symbol is
 * encountered in another translation unit, the weak symbol will be replaced by
 * the global symbol.
 *
 */
enum class Binding {
  kLocal,    /*!< Local definitons allows a symbol to be declared in every
                translation unit.*/
  kGlobal,   /*!< Global definiton requires a symbol be defined in only one
                translation unit.*/
  kImported, /*!< A global symbol that has been taken from another translation
                unit.*/
};
Q_ENUM_NS(Binding);

enum class TraversalPolicy {
  kChildren = 0,     /*!< Only visit the current node and its children.*/
  kChildrenOnly = 1, /*< Only visit children of the current node */
  kSiblings = 2,     /*!< Visit the current node, its children, its siblings, and
                        its siblings children. That is, the parent's children.*/
  kWholeTree = 3,    /*!< Visit every node in the tree.*/
};
Q_ENUM_NS(TraversalPolicy)

}; // end namespace symbol
