/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
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
#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include "../../shared.hpp"
#include "../../task.hpp"

// A subset of GNU ar supporting operations d, p, q, r, s, t, and x with modifiers c, D, s, S, T, and v. Archives are
// always deterministic, implying D
class ArTask : public Task {
public:
  // Index is s on its own, which rebuilds the index as ranlib does.
  enum class Operation : char {
    Delete = 'd',
    Print = 'p',
    QuickAppend = 'q',
    Replace = 'r',
    Index = 's',
    List = 't',
    Extract = 'x',
  };
  // The command line
  struct Command {
    std::optional<Operation> operation;
    bool quiet_create = false, index = false, no_index = false, thin = false, verbose = false;
    std::string archive;
    std::vector<std::string> members;
  };
  ArTask(Command &command, QObject *parent = nullptr);
  void run() override;

private:
  Command &_command;
};

// GNU allows ar's keys without a leading `-`, as in `ar rcs`.
// Rewrite them as `-rcs` so that they parse as flags rather than positionals.
inline void dash_ar_keys(std::vector<const char *> &args) {
  static std::string dashed;
  const auto ar = std::ranges::find_if(args, [](std::string_view arg) { return arg == "ar" || arg == "par"; });
  if (ar == args.end() || ar + 1 == args.end() || *ar[1] == '-') return;
  dashed = std::string("-") + ar[1];
  ar[1] = dashed.c_str();
}

void registerAr(auto &app, task_factory_t &task, detail::SharedFlags &flags) {
  using enum ArTask::Operation;
  static ArTask::Command command;
  static auto ar = app.add_subcommand("ar", "Create, modify, and extract from archives.")->alias("par");
  // As in ar(1): the first argument is one operation key followed by any modifiers, optionally after a `-`.
  ar->usage("ar [-]{dpqrstx}[cDsSTv] [--thin] archive [file...]");
  static auto operations = ar->add_option_group("Operations");
  const auto operation = [&](const char *flag, ArTask::Operation value, const char *description) {
    operations->add_flag_callback(flag, [value] { command.operation = value; }, description);
  };
  operation("-d", Delete, "Delete files from the archive. A no-op when no files are specific");
  operation("-p", Print, "Print files found in the archive to stdout.");
  operation("-q", QuickAppend, "Append the files without checking for members of the same name.");
  operation("-r", Replace, "Insert the files, replacing existing members of the same name.");
  operation("-t", List, "List the archive's contents. With -v, also each member's mode, owner, date, and size.");
  operation("-x", Extract, "Extract the named members.");
  operations->require_option(0, 1);
  static auto modifiers = ar->add_option_group("Modifiers");
  modifiers->add_flag("-c", command.quiet_create, "Create the archive without warning that it had to be created.");
  modifiers->add_flag("-s", command.index, "Write or update the symbol index, with modified members");
  modifiers->add_flag("-S", command.no_index, "Do not write a symbol index.");
  modifiers->add_flag("-T,--thin", command.thin, "Make a thin archive, which refers to files by path.");
  modifiers->add_flag("-v", command.verbose, "Verbose output.");
  modifiers->add_flag("-D", "Deterministic mode: zero uids, gids, and dates.");
  ar->add_option("archive", command.archive, "The archive to create, modify, or read.")->required();
  ar->add_option("files", command.members, "Files to add, remove, or modify. None means every member of the archive.");
  ar->callback([&]() {
    // treat -s as an operation if no other operation was specified.
    if (!command.operation && command.index) command.operation = Index;
    flags.kind = detail::SharedFlags::Kind::TERM;
    task = [&](QObject *parent) { return new ArTask(command, parent); };
  });
}
