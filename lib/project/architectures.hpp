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

#include <QString>
#include <QtCore>
#include <QtQmlIntegration>
#include "core/architectures.hpp"

// Must be in separate file to prevent circuluar include in Qt MOC.
namespace pepp {

class ArchitectureHelper : public QObject {
  Q_GADGET
  QML_NAMED_ELEMENT(Architecture)
  QML_UNCREATABLE("Error:Only enums")

public:
  // Have to shadow the earlier enum, because Q_ENUM only works enums declared inside this class.
  enum class Architecture {
    NO_ARCH = (int)pepp::Architecture_Enum::NO_ARCH,
    PEP8 = (int)pepp::Architecture_Enum::PEP8,
    PEP9 = (int)pepp::Architecture_Enum::PEP9,
    PEP10 = (int)pepp::Architecture_Enum::PEP10,
    RISCV = (int)pepp::Architecture_Enum::RISCV,
  };
  Q_ENUM(Architecture)
  ArchitectureHelper(QObject *parent = nullptr);
  Q_INVOKABLE static QString string(int architecture);
};

} // namespace pepp
