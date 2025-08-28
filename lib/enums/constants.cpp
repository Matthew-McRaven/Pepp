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
#include "constants.hpp"
pepp::AbstractionHelper::AbstractionHelper(QObject *parent) : QObject(parent) {}

QString pepp::AbstractionHelper::string(Abstraction abstraction) const {
  QMetaEnum metaEnum = QMetaEnum::fromType<Abstraction>();
  return metaEnum.valueToKey(static_cast<int>(abstraction));
}

pepp::ArchitectureHelper::ArchitectureHelper(QObject *parent) : QObject(parent) {}

QString pepp::ArchitectureHelper::string(Architecture architecture) {
  QMetaEnum metaEnum = QMetaEnum::fromType<Architecture>();
  return QString(metaEnum.valueToKey(static_cast<int>(architecture)));
}

pepp::ArchitectureUtils::ArchitectureUtils(QObject *parent) : QObject(parent) {}

QString pepp::ArchitectureUtils::archAsString(Architecture architecture) {
  return ArchitectureHelper::string(architecture);
}

QString pepp::archAsPrettyString(ArchitectureHelper::Architecture architecture) {
  switch (architecture) {
  case ArchitectureHelper::Architecture::PEP8: return "Pep/8";
  case ArchitectureHelper::Architecture::PEP9: return "Pep/9";
  case ArchitectureHelper::Architecture::PEP10: return "Pep/10";
  case ArchitectureHelper::Architecture::RISCV: return "RISC-V";
  }
  qDebug() << "Unknown architecture in archAsPrettyString";
  return "Unknown";
}
