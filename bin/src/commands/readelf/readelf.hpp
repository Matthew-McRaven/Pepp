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
    bool debug = false;
    std::string elffile;
  };
  ReadElfTask(Options &opts, QObject *parent = nullptr);
  void run() override;

private:
  void fileHeader(ELFIO::elfio &) const;
  void programHeaders(ELFIO::elfio &) const;
  void sectionHeaders(ELFIO::elfio &) const;
  void symbols(ELFIO::elfio &) const;
  void notes(ELFIO::elfio &) const;
  void debug(ELFIO::elfio &) const;
  Options &_opts;
};

void registerReadelf(auto &app, task_factory_t &task, detail::SharedFlags &flags) {
  static ReadElfTask::Options opts;
  static auto readelf = app.add_subcommand("readelf", "Display information about ELF files");
  readelf->set_help_flag("-H,--help");
  static auto file_header = readelf->add_flag("-h,--file-header", opts.file_header, "Display the ELF file header");
  static auto program_headers =
      readelf->add_flag("-l,--program-headers", opts.program_headers, "Display the program headers");
  static auto section_headers =
      readelf->add_flag("-S,--section-headers", opts.section_headers, "Display the section headers");
  static auto symbols = readelf->add_flag("-s,--symbols", opts.symbols, "Display the symbol tables");
  static auto notes = readelf->add_flag("-n,--notes", opts.notes, "Display the notes");
  static auto debug = readelf->add_flag("-d,--debug", opts.debug, "Display Pepp specific debug info");
  static auto all = readelf->add_flag("-a,--all", " Equivalent to specifying --file-header, --program-headers,\
--sections, --symbols, --debug, --relocs, --dynamic, --notes,\
--version-info, --arch-specific, --unwind, --section-groups.");
  static auto file = readelf->add_option("elffile", opts.elffile, "Elf file")->expected(1)->required(true);
  readelf->callback([&]() {
    if (*all) {
      opts.file_header = opts.program_headers = opts.section_headers = opts.symbols = opts.notes = opts.debug = true;
    }
    flags.kind = detail::SharedFlags::Kind::TERM;
    task = [&](QObject *parent) { return new ReadElfTask(opts, parent); };
  });
}
