/*
 * Copyright (c) 2023-2026 J. Stanley Warford, Matthew McRaven
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
#include "core/libs/bitmanip/span.hpp"
#include "core/libs/bitmanip/strings.hpp"
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
  if (_opts.relocs) relocs(elf);
  if (_opts.debug_line) debug_line(elf);
  if (_opts.debug_info) debug_info(elf);
  if (!_opts.dump_strs.empty()) dump_strs(elf);
  if (!_opts.dump_hex.empty()) dump_hex(elf);
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

void ReadElfTask::relocs(ELFIO::elfio &elf) const {
  using namespace Qt::StringLiterals;
  for (const auto &sec : elf.sections) {
    // Only process relocation sections. sh_info is not guaranteed to be valid, so we need to ensure it is both in range
    // and points to a symbol table.
    if (sec->get_type() != ELFIO::SHT_REL && sec->get_type() != ELFIO::SHT_RELA) continue;
    else if (sec->get_link() > elf.sections.size()) {
      std::cout << fmt::format("Relocation section '{}' has invalid sh_link {}", sec->get_name(), sec->get_link())
                << std::endl;
      continue;
    }
    auto sec_symtab = elf.sections[sec->get_link()];
    if (sec_symtab->get_type() != ELFIO::SHT_SYMTAB) {
      std::cout << fmt::format("Relocation section '{}' sh_link {} does not point to symbol table", sec->get_name(),
                               sec->get_link())
                << std::endl;
      continue;
    }

    auto sym_acc = ELFIO::symbol_section_accessor(elf, sec_symtab);
    auto rel_acc = ELFIO::relocation_section_accessor(elf, sec.get());
    std::cout << fmt::format("Relocation section '{}' at offset 0x{:x} contains {} {}", sec->get_name(),
                             sec->get_offset(), rel_acc.get_entries_num(),
                             rel_acc.get_entries_num() == 1 ? "entry" : "entries")
              << std::endl;

    if (sec->get_type() == ELFIO::SHT_REL)
      std::cout << fmt::format("{:^8} {:^8} {:^16} {:^8} {}", "Offset", "Info", "Type", "Sym.Value", "Sym.Name")
                << std::endl;
    else
      std::cout << fmt::format("{:^8} {:^8} {:^16} {:^8} {:^8} {} ", "Offset", "Info", "Type", "Addend", "Sym.Value",
                               "Sym.Name")
                << std::endl;

    // So many unused temporaries, but that's the API for ELFIO.
    for (int rel_idx = 0; rel_idx < rel_acc.get_entries_num(); rel_idx++) {
      ELFIO::Elf64_Addr rel_offset;
      ELFIO::Elf_Word rel_symbol;
      unsigned rel_type;
      ELFIO::Elf_Sxword rel_addened;
      if (!rel_acc.get_entry(rel_idx, rel_offset, rel_symbol, rel_type, rel_addened)) {
        std::cout << fmt::format("Invalid entry at index {}", rel_idx);
        continue;
      }
      auto rel_info =
          (((rel_symbol) << 8) + (unsigned char)(rel_type)); // Recompute r_info according to Fig 1-22 of ELF spec

      std::string sym_name;
      ELFIO::Elf64_Addr sym_value;
      ELFIO::Elf_Xword sym_size;
      unsigned char sym_bind, sym_type, sym_other;
      ELFIO::Elf_Half section_index;
      // Do not skip printing on invalid symbol, but warn user that they might want to inspect r_info.
      if (!sym_acc.get_symbol((ELFIO::Elf_Xword)rel_symbol, sym_name, sym_value, sym_size, sym_bind, sym_type,
                              section_index, sym_other)) {
        std::cout << fmt::format("Invalid symbol, check info") << std::endl;
        sym_name = "", sym_value = 0;
      }

      // TODO: attempt to decode rel_type to a string based on the architecture.
      if (sec->get_type() == ELFIO::SHT_REL)
        std::cout << fmt::format("{:08x} {:08x} {:^16} {:08x}  {}", rel_offset, rel_info, rel_type, sym_value, sym_name)
                  << std::endl;
      else
        std::cout << fmt::format("{:08x} {:08x} {:^16} {:08x} {:08x}  {}", rel_offset, rel_info, rel_type, rel_addened,
                                 sym_value, sym_name)
                  << std::endl;
    }
    std::cout << std::endl;
  }
}

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
  for (const auto &item : _opts.dump_strs) {
    auto sec_idx = QString::fromStdString(item).toUInt(&okay);
    if (okay) {
      if (sec_idx > 0 && sec_idx < elf.sections.size()) dump_strings(elf.sections[sec_idx]);
      else std::cerr << "No such section index: " << sec_idx << std::endl;
    } else {
      for (const auto &sec : std::as_const(elf.sections)) {
        if (sec->get_name() == item) dump_strings(sec.get());
      }
    }
  }
}

// Must also update format placeholder(s) below
static constexpr int hex_bytes_per_row = 16;
QList<bits::SeparatorRule> hex_rules = {{.skipFirst = true, .separator = ' ', .modulus = hex_bytes_per_row / 4}};
size_t print_one_hex_row(bits::span<const quint8> data, size_t offset) {
  if (data.empty()) return 0;
  auto n = std::min<size_t>(hex_bytes_per_row, data.size());

  char hex_bytes[hex_bytes_per_row * 2 + 3];
  auto hex_bytes_span = bits::span<char>(hex_bytes);
  auto hex_end = bits::bytesToAsciiHex(hex_bytes_span, data.first(n), hex_rules);
  char ascii_bytes[hex_bytes_per_row];
  auto ascii_bytes_span = bits::span<char>(ascii_bytes);
  auto ascii_end = bits::bytesToPrintableAscii(ascii_bytes_span, data.first(n), {});
  std::cout << fmt::format("  0x{:08x} {:35} {:16}", offset, std::string_view(hex_bytes, hex_end),
                           std::string_view(ascii_bytes, ascii_end))
            << std::endl;
  return n;
}

void dump_hex_helper(const ELFIO::section *sec) {
  using namespace Qt::StringLiterals;
  std::cout << "Hex dump of section (" << sec->get_name() << ")" << std::endl;
  const char *data = sec->get_data();
  auto size = sec->get_size();
  if (data == nullptr) return;

  auto span = bits::span<const quint8>{reinterpret_cast<const quint8 *>(data), (size_t)size};
  for (int it = 0; it < size; it += hex_bytes_per_row) span = span.subspan(print_one_hex_row(span, it));
  std::cout << std::endl;
}

void ReadElfTask::dump_hex(ELFIO::elfio &elf) const {
  bool okay = false;
  for (const auto &item : _opts.dump_hex) {
    auto sec_idx = QString::fromStdString(item).toUInt(&okay);
    if (okay) {
      if (sec_idx > 0 && sec_idx < elf.sections.size()) dump_hex_helper(elf.sections[sec_idx]);
      else std::cerr << "No such section index: " << sec_idx << std::endl;
    } else {
      for (const auto &sec : std::as_const(elf.sections)) {
        if (sec->get_name() == item) dump_hex_helper(sec.get());
      }
    }
  }
}
