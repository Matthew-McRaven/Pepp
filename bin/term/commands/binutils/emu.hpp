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
#include <vector>
#include "../../shared.hpp"
#include "../../task.hpp"
#include "../name_value.hpp"
#include "core/integers.h"

class PeppEmulator : public Task {
public:
  enum class SystemEnu { RV32I, Pep10OS, Pep10BM };
  struct Options {
    // If a string, treat it as a path to a JSON file which should be parsed to create the system.
    std::variant<SystemEnu, std::string> system = SystemEnu::Pep10OS;
    // Named registers/fields set initialization and their values. Negative values stored as two's complement.
    std::vector<std::pair<std::string, u64>> set_registers;
    // Named registers/fields printed to stdout at the end of execution.
    std::vector<std::string> print_registers;
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
  static std::vector<std::string> set_reg_text;
  // One value per occurrence, so a following positional argument is not taken as a second register.
  pemu->add_option("--set-reg", set_reg_text,
                   "Set a register or field after the system is initialized, with a signed decimal, unsigned "
                   "decimal, or 0x-prefixed hex value. May be repeated.")
      ->option_text("<name>=<value>")
      ->allow_extra_args(false)
      ->take_all()
      ->check(CLI::Validator(
          [](std::string &arg) -> std::string {
            return parse_name_value<u64>(arg) ? "" : "expected <name>=<integer value>";
          },
          ""));
  pemu->add_option(
          "--print-reg", opts.print_registers,
          "Print a register or field as <name>=<hex value> after the simulated program terminates. May be repeated.")
      ->option_text("<name>")
      ->allow_extra_args(false)
      ->take_all();

  pemu->callback([&]() {
    opts.set_registers.clear();
    for (const auto &arg : set_reg_text) opts.set_registers.push_back(*parse_name_value<u64>(arg));
    if (system_json_opt->count() > 0) opts.system = system_json;
    else opts.system = system;
    flags.kind = detail::SharedFlags::Kind::TERM;
    task = [&](QObject *parent) { return new PeppEmulator(opts, parent); };
  });
}
