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

#include <QString>
#include <QtCore>
#include <QtQmlIntegration>

// Must be in separate file to prevent circuluar include in Qt MOC.
namespace pepp {
Q_NAMESPACE;

class AbstractionHelper : public QObject {
  Q_GADGET
  QML_NAMED_ELEMENT(Abstraction)
  QML_UNCREATABLE("Error:Only enums")

public:
  enum class Abstraction {
    NO_ABS = -1,
    // LG1 = 1,
    MC2 = 20,
    ISA3 = 30,
    ASMB3 = 31,
    OS4 = 40,
    ASMB5 = 50,
    // HOL6 = 6,
    // APP7 = 7,
  };
  Q_ENUM(Abstraction)
  AbstractionHelper(QObject *parent = nullptr);
  Q_INVOKABLE QString string(Abstraction abstraction) const;
};
using Abstraction = AbstractionHelper::Abstraction;

class ArchitectureHelper : public QObject {
  Q_GADGET
  QML_NAMED_ELEMENT(Architecture)
  QML_UNCREATABLE("Error:Only enums")

public:
  enum class Architecture {
    NO_ARCH = -1, //! Architecture is unspecified.
    PEP8 = 80,    //! The figure must be used with the Pep/8 toolchain.
    PEP9 = 90,    //! The figure must be used with the Pep/9 toolchain.
    PEP10 = 100,  //! The figure must be use with the Pep/10 toolchain
    RISCV = 1000, //! The figure must be used with the RISC-V toolchain, which is
                  //! undefined as of 2023-02-14.
  };
  Q_ENUM(Architecture)
  ArchitectureHelper(QObject *parent = nullptr);
  Q_INVOKABLE static QString string(Architecture architecture);
};
using Architecture = ArchitectureHelper::Architecture;
class ArchitectureUtils : public QObject {
  Q_OBJECT
  QML_ELEMENT
public:
  ArchitectureUtils(QObject *parent = nullptr);
  Q_INVOKABLE QString archAsString(Architecture architecture);
};

} // namespace pepp
