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
#include <QtCore>
#include <elfio/elfio.hpp>

namespace obj {

// May be nullptr if no MMIO note section.
const ELFIO::section *getMMIONoteSection(const ELFIO::elfio &elf);
// If no MMIO note section, will create.
ELFIO::section *addMMIONoteSection(ELFIO::elfio &elf);

struct IO {
  QString name;
  enum class Type { kInput, kOutput, kIDE } type;
};
void addMMIODeclarations(ELFIO::elfio &elf, ELFIO::section *symTab, QList<IO> mmios);
// Mark the following symbol as the starting symbol for an memory-mapped IDE controller.
void addIDEDeclaration(ELFIO::elfio &elf, ELFIO::section *symTab, QString symbol);

struct AddressedIO : public IO {
  quint16 minOffset, maxOffset;
};

QList<AddressedIO> getMMIODeclarations(const ELFIO::elfio &elf);
} // namespace obj
