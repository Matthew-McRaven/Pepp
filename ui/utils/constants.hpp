/*
 * Copyright (c) 2024 J. Stanley Warford, Matthew McRaven
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
#include <QObject>
#include "utils_global.hpp"

namespace utils {
class UTILS_EXPORT Abstraction : public QObject {
  Q_GADGET
public:
  Abstraction(QObject *parent = nullptr);
  enum Value {
    // LG1 = 1,
    MC2 = 2,
    ISA3 = 3,
    OS4 = 4,
    ASMB5 = 5,
    // HOL6 = 6,
    // APP7 = 7,
  };
  Q_ENUM(Value);
};

class UTILS_EXPORT Architecture : public QObject {
  Q_GADGET
public:
  Architecture(QObject *parent = nullptr);
  enum Value {
    // Pep8 = 8,
    // Pep9 = 9,
    Pep10 = 10,
    RISCV32I = 128,
  };
  Q_ENUM(Value);
};
} // namespace utils
