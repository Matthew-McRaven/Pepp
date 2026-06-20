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

class AbstractionHelper : public QObject {
  Q_GADGET
  QML_NAMED_ELEMENT(Abstraction)
  QML_UNCREATABLE("Error:Only enums")

public:
  // If you find yourself accessing this enum from C++, stop. It's just a hack to make the constants available on a
  // singleton in QML
  enum class OnlyUsableFromQML_Abstraction {
    NO_ABS = (int)pepp::Abstraction::NO_ABS,
    // LG1 = 1,
    MA2 = (int)pepp::Abstraction::MA2,
    ISA3 = (int)pepp::Abstraction::ISA3,
    ASMB3 = (int)pepp::Abstraction::ASMB3,
    OS4 = (int)pepp::Abstraction::OS4,
    ASMB5 = (int)pepp::Abstraction::ASMB5,
    // HOL6 = 6,
    // APP7 = 7,
  };
  Q_ENUM(OnlyUsableFromQML_Abstraction)
  AbstractionHelper(QObject *parent = nullptr);
  Q_INVOKABLE QString string(int abstraction) const;
};
} // namespace pepp
