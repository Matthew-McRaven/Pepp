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

#include "builtins_globals.hpp"
#include <QtCore>

// Must be in separate file to prevent circuluar include in Qt MOC.
namespace builtins {
Q_NAMESPACE_EXPORT(BUILTINS_EXPORT)
//! Describe which architecture a help item is to be used with.
enum class Architecture {
  NONE = -1,    //! Architecture is unspecified.
  PEP8 = 80,    //! The figure must be used with the Pep/8 toolchain.
  PEP9 = 90,    //! The figure must be used with the Pep/9 toolchain.
  PEP10 = 100,  //! The figure must be use with the Pep/10 toolchain
  RISCV = 1000, //! The figure must be used with the RISC-V toolchain, which is
                //! undefined as of 2023-02-14.
};
Q_ENUM_NS(Architecture);
enum class Abstraction {
  NONE = -1,
  // LG1 = 1,
  MC2 = 2,
  ISA3 = 3,
  OS4 = 4,
  ASMB5 = 5,
  // HOL6 = 6,
  // APP7 = 7,
};
Q_ENUM_NS(Abstraction);
} // end namespace builtins
