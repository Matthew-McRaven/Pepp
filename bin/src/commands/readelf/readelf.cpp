/*
 * Copyright (c) 2023-2024 J. Stanley Warford, Matthew McRaven
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

#include "readelf.hpp"
#include <elfio/elfio.hpp>
#include <elfio/elfio_dump.hpp>
#include <iostream>
#include "builtins/figure.hpp"
#include "helpers/asmb.hpp"

ReadElfTask::ReadElfTask(Options &opts, QObject *parent) : Task(parent), _opts(opts) {}

void ReadElfTask::run() {
  using namespace Qt::StringLiterals;
  auto elf = ELFIO::elfio{};
  if (!elf.load(_opts.elffile)) {
    qWarning() << "Failed to open elf file: " << _opts.elffile;
    return emit finished(1);
  }
  if (_opts.file_header) fileHeader(elf);
  if (_opts.section_headers) sectionHeaders(elf);
  if (_opts.program_headers) programHeaders(elf);
  if (_opts.symbols) symbols(elf);
  if (_opts.notes) notes(elf);

  return emit finished(0);
}

void ReadElfTask::fileHeader(ELFIO::elfio &elf) const {
  /* Missing the following fields:
  Start of program headers:          52 (bytes into file)
  Start of section headers:          4896 (bytes into file)
  Flags:                             0x0
  Size of this header:               52 (bytes)
  Size of program headers:           32 (bytes)
  Number of program headers:         5
  Size of section headers:           40 (bytes)
  Number of section headers:         10
  Section header string table index: 1
*/
  ELFIO::dump::header(std::cout, elf);
}

void ReadElfTask::programHeaders(ELFIO::elfio &elf) const { ELFIO::dump::segment_headers(std::cout, elf); }

void ReadElfTask::sectionHeaders(ELFIO::elfio &elf) const { ELFIO::dump::section_headers(std::cout, elf); }

void ReadElfTask::symbols(ELFIO::elfio &elf) const {
  // Symbols relative to ABS and UND should use that string, not a number.
  ELFIO::dump::symbol_tables(std::cout, elf);
}

void ReadElfTask::notes(ELFIO::elfio &elf) const { ELFIO::dump::notes(std::cout, elf); }
