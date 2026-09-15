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
#include <string>
#include <variant>
#include "../../shared.hpp"
#include "../../task.hpp"

class PeppEmulator : public Task {
public:
  enum class SystemEnu { RV32I, Pep10OS, Pep10BM };
  struct Options {
    // If a string, treat it as a path to a JSON file which should be parsed to create the system.
    std::variant<SystemEnu, std::string> system = SystemEnu::Pep10OS;
  };
  PeppEmulator(Options &opts, QObject *parent = nullptr);
  void run() override;

private:
  Options &_opts;
};

void registerEmu(auto &app, task_factory_t &task, detail::SharedFlags &flags) {
  static PeppEmulator::Options opts;
  using SystemEnu = PeppEmulator::SystemEnu;
  static SystemEnu system = SystemEnu::Pep10OS;
  static std::string system_json;
  static auto pemu = app.add_subcommand("emu", "new simulator")->alias("pemu");
  static auto system_group = pemu->add_option_group("System", "Select the simulated system")->require_option(0, 1);
  system_group->add_option("--system", system, "Use a built-in system")
      ->transform(CLI::CheckedTransformer(std::map<std::string, SystemEnu>{{"rv32i", SystemEnu::RV32I},
                                                                           {"pep10.os", SystemEnu::Pep10OS},
                                                                           {"pep10.bm", SystemEnu::Pep10BM}},
                                          CLI::ignore_case));
  static auto system_json_opt =
      system_group->add_option("--system-json", system_json, "Use a custom system")->option_text("<name>");

  pemu->callback([&]() {
    if (system_json_opt->count() > 0) opts.system = system_json;
    else opts.system = system;
    flags.kind = detail::SharedFlags::Kind::TERM;
    task = [&](QObject *parent) { return new PeppEmulator(opts, parent); };
  });
}
