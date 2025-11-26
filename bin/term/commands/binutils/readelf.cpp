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
#include "help/builtins/figure.hpp"
#include "toolchain/helpers/asmb.hpp"
#include "toolchain/pas/obj/common.hpp"
#include "toolchain/pas/obj/trace_tags.hpp"

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
  if (_opts.debug_line) debug_line(elf);
  if (_opts.debug_info) debug_info(elf);
  if (!_opts.dump_strs.empty()) dump_strs(elf);
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

void ReadElfTask::debug_line(ELFIO::elfio &elf) const {
  static const int columns = 8;
  using namespace Qt::StringLiterals;
  auto sec = pas::obj::common::detail::getLineMappingSection(elf);
  if (sec == nullptr) return;
  std::cout << "Line mapping section (" << sec->get_name() << ")" << std::endl;
  auto linemaps = pas::obj::common::getLineMappings(elf);
  std::sort(linemaps.begin(), linemaps.end());
  int ctr = 0;
  std::cout << "  ";
  for (const auto &map : linemaps) {
    std::cout << ((QString)map).toStdString();
    if (ctr++ % columns == columns - 1) std::cout << std::endl << "  ";
    else std::cout << " ";
  }
  if (ctr % columns != 0) std::cout << std::endl;
  std::cout << std::endl;
}

void ReadElfTask::debug_info(ELFIO::elfio &elf) const {
  auto sec = pas::obj::common::detail::getTraceSection(elf);
  if (sec == nullptr) return;
  std::cout << "Debug info section (" << sec->get_name() << ")" << std::endl;
  auto info = pas::obj::common::readDebugCommands(elf);
  for (const auto &[addr, frame] : std::as_const(info.commands).asKeyValueRange()) {
    std::cout << fmt::format("  {:04x}: {}\n", addr, QString(frame).toStdString());
  }
  std::cout << std::endl << std::endl;
}
using Elf_Xword = ELFIO::Elf_Xword;
void print_one(Elf_Xword idx, std::string_view sv) { std::cout << fmt::format("  [{:04x}] {}\n", idx, sv); }
void dump_strings(const ELFIO::section *sec) {
  using namespace Qt::StringLiterals;
  std::cout << "String dump of section (" << sec->get_name() << ")" << std::endl;
  const char *data = sec->get_data();
  if (data == nullptr) return;
  auto size = sec->get_size();
  std::optional<int> start_idx = std::nullopt;

  // TODO: probably should handle non-printable characters too.
  for (Elf_Xword i = 0; i < size; ++i) {
    if (data[i] == '\0') {
      if (start_idx && i > start_idx) {
        std::string_view sv(data + *start_idx, i - *start_idx);
        print_one(*start_idx, sv);
      }
      start_idx = std::nullopt;
    } else {
      if (!start_idx) start_idx = i;
    }
  }
  if (start_idx && size > *start_idx) {
    std::string_view sv(data + *start_idx, size - *start_idx);
    print_one(*start_idx, sv);
  }
  std::cout << std::endl;
}
void ReadElfTask::dump_strs(ELFIO::elfio &elf) const {
  bool okay = false;
  auto sec_idx = QString::fromStdString(_opts.dump_strs).toUInt(&okay);
  if (okay) {
    if (sec_idx > 0 && sec_idx < elf.sections.size()) dump_strings(elf.sections[sec_idx]);
    else std::cerr << "No such section index: " << sec_idx << std::endl;
  } else {
    for (const auto &sec : std::as_const(elf.sections)) {
      if (sec->get_name() == _opts.dump_strs) dump_strings(sec.get());
    }
  }
}
