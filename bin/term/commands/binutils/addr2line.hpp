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

#pragma once
#include <CLI11.hpp>
#include "../../shared.hpp"
#include "../../task.hpp"

namespace ELFIO {
class elfio;
}

class Addr2LineTask : public Task {
public:
  struct Options {
    bool listing_numbers = false;
    std::vector<int> addresses;
    std::string elffile;
  };
  Addr2LineTask(Options &opts, QObject *parent = nullptr);
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

void registerAddr2Line(auto &app, task_factory_t &task, detail::SharedFlags &flags) {
  static Addr2LineTask::Options opts;
  // TODO: when "source file" info is added to ELF file, prepend output with that info.
  static auto addr2line = app.add_subcommand("addr2line", "convert addresses into line numbers");
  /*static auto section_headers =
      addr2line->add_flag("-j,--section", opts.section_offsets,
                          "Read offsets relative to the specified section instead of absolute addresses.");*/
  static auto listing_number =
      addr2line->add_flag("-l,--listing", opts.listing_numbers,
                          "Use listing line numbers rather than source line numbers when translating addresses.");
  static auto file = addr2line
                         ->add_flag("-e,--exe", opts.elffile,
                                    "Specify the name of the executable for which addresses should be translated.")
                         ->expected(1)
                         ->required(true);
  addr2line->add_option("addresses", opts.addresses,
                        "A list of addresses (in hex, e.g. 0x1a2b) to be translated to file names and line numbers.");

  addr2line->callback([&]() {
    flags.kind = detail::SharedFlags::Kind::TERM;
    task = [&](QObject *parent) { return new Addr2LineTask(opts, parent); };
  });
}
