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
#include "core/integers.h"
namespace pepp::core::symbol {
/*!
 * The definition_state of a symbol tracks the number of times a symbol has been
 * defined within a translation unit.
 *
 * Multiply defined symbols should cause assembly or linkage to fail very
 * quickly, as there is no way to recover.
 */
enum class DefinitionState : u8 {
  Undefined,        // A symbol that is not defined and referenced 1+ times.
  Single,           // A symbol defined once and referenced 0+ times.
  Multiple,         // A symbol defined 2+ times and referenced 0+ times.
  ExternalMultiple, // A multiply-defined symbol, with at least one definition being from an external TU
};

// These fields roughly matp to ELF symbol types, with some added entries for ease of writing our assembler.
// Those extensions
enum class Type : u8 {
  /*! Represent a not-yet-defined symbol's value.
   * Maps to ELF's STT_NOTYPE.*/
  Empty,
  /*! The associated symbol is an address in memory of data.
   * These types arise from symbol declarations like: \code{.s}hi:.WORD
   * 2\endcode Maps to ELF's STT_OBJECT. */
  Object,
  /*! The associated symbol represents an address in memory of code.
   * These types arise from symbol declarations like: \code{.s}hi:NOP\endcode
   * Maps to ELF's STT_FUNC. */
  Code,
  /*! The associated symbol represents an numeric value or string in memory.
   * These types arise from symbol declarations like: \code{.asm}hi:.EQUATE
   * 10\endcode Maps to ELF's STT_ABS.*/
  Constant,
  /*! The associated symbol has been marked for deletion.
   * Does not map to any ELF symbol type, as this symbol no longer exists.*/
  Deleted,
  /*! This symbol is an alias for another symbol.*/
  Alias,
};

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
enum class Binding : u8 {
  Local,  // Local definitons allows a symbol to be declared in every translation unit.
  Global, // Requires a symbol be defined in only one translation unit.
  Weak    // A global definition that may be overridden by a Global
};

} // namespace pepp::core::symbol
