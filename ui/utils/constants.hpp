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
#include <qqmlintegration.h>
#include "help/builtins/utils.hpp"
#include "utils_global.hpp"

namespace utils {
using Architecture = builtins::Architecture;
using Abstraction = builtins::Abstraction;
class UTILS_EXPORT AbstractionHelper : public QObject {
  Q_GADGET
  QML_NAMED_ELEMENT(Abstraction)
public:
  AbstractionHelper(QObject *parent = nullptr);
  Q_ENUM(builtins::Abstraction);
};

class UTILS_EXPORT ArchitectureHelper : public QObject {
  Q_GADGET
  QML_NAMED_ELEMENT(Architecture)
public:
  ArchitectureHelper(QObject *parent = nullptr);
  Q_ENUM(builtins::Architecture);
};
} // namespace utils
