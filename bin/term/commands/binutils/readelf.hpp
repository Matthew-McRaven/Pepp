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

#pragma once
#include <CLI11.hpp>
#include "../../shared.hpp"
#include "../../task.hpp"

namespace ELFIO {
class elfio;
}

class ReadElfTask : public Task {
public:
  struct Options {
    bool file_header = false;
    bool program_headers = false;
    bool section_headers = false;
    bool symbols = false;
    bool notes = false;
    bool debug_line = false;
    bool debug_info = false;
    std::string elffile;
    std::string dump_strs;
  };
  ReadElfTask(Options &opts, QObject *parent = nullptr);
  void run() override;

private:
  void fileHeader(ELFIO::elfio &) const;
  void programHeaders(ELFIO::elfio &) const;
  void sectionHeaders(ELFIO::elfio &) const;
  void symbols(ELFIO::elfio &) const;
  void notes(ELFIO::elfio &) const;
  void debug_line(ELFIO::elfio &) const;
  void debug_info(ELFIO::elfio &) const;
  void dump_strs(ELFIO::elfio &) const;
  Options &_opts;
};

void registerReadelf(auto &app, task_factory_t &task, detail::SharedFlags &flags) {
  static ReadElfTask::Options opts;
  static auto readelf = app.add_subcommand("readelf", "Display information about ELF files");
  readelf->set_help_flag("-H,--help");
  static auto file_header = readelf->add_flag("-h,--file-header", opts.file_header, "Display the ELF file header");
  static auto program_headers =
      readelf->add_flag("-l,--program-headers", opts.program_headers, "Display the program headers");
  static auto dump_strs = readelf->add_option("-p,--string-dump", opts.dump_strs,
                                              "Displays the contents of the indicated section as printable strings.\
A number identifies a particular section by index in the section table; any other string identifies all sections\
with that name in the object file.");
  static auto section_headers =
      readelf->add_flag("-S,--section-headers", opts.section_headers, "Display the section headers");
  static auto symbols = readelf->add_flag("-s,--symbols", opts.symbols, "Display the symbol tables");
  static auto notes = readelf->add_flag("-n,--notes", opts.notes, "Display the notes");
  // -wli would be the standard GNU options, but I don't want to figure out how to parse those right now.
  // For full compatibility, we would need to support these options as -wl, -wi, and -wli.
  static auto debug_line = readelf->add_flag("--wl", opts.debug_line, "Display debugger line numbers");
  static auto debug_info = readelf->add_flag("--wi", opts.debug_info, "Display debugger trace info");
  static auto headers =
      readelf->add_flag("-e,--headers", " Display all the headers in the file.  Equivalent to -h -l -S");
  static auto all = readelf->add_flag("-a,--all", " Equivalent to specifying --file-header, --program-headers,\
--sections, --symbols, --relocs, --dynamic, --notes");
  static auto file = readelf->add_option("elffile", opts.elffile, "Elf file")->expected(1)->required(true);
  readelf->callback([&]() {
    if (*all) {
      opts.file_header = opts.program_headers = opts.section_headers = opts.symbols = opts.notes = true;
    }
    if (*headers) {
      opts.file_header = opts.program_headers = opts.section_headers = true;
    }
    flags.kind = detail::SharedFlags::Kind::TERM;
    task = [&](QObject *parent) { return new ReadElfTask(opts, parent); };
  });
}
