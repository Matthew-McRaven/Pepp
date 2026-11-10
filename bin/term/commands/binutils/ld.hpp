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

// Help text derived from the contents of `man ld`

#pragma once
#include <CLI11.hpp>
#include <filesystem>
#include <string>
#include <vector>
#include "../../shared.hpp"
#include "../../task.hpp"
#include "core/link/link_config.hpp"

class LdTask : public Task {
public:
  LdTask(pepp::link::LinkConfig &config, QObject *parent = nullptr);
  void run() override;

private:
  pepp::link::LinkConfig &_config;
};

void registerLd(auto &app, task_factory_t &task, detail::SharedFlags &flags) {
  static pepp::link::LinkConfig config;
  static CLI::App *ld = app.add_subcommand("ld", "Link object files and archives.")->alias("pld");
  ld->add_option_function<std::string>(
      "-o,--output", [](const std::string &path) { config.output = path; },
      "Set output file name")
      ->option_text("FILE");
  ld->add_option("-e,--entry", config.entry, "Set start address")->option_text("ADDRESS");
  ld->add_option("-m", config.emulation, "Set emulation")->option_text("EMULATION");
  ld->add_option_function<std::vector<std::string>>(
      "--section-start",
      [](const std::vector<std::string> &args) {
        for (const auto &arg : args)
          if (auto parsed = pepp::link::parse_section_start(arg)) config.section_start.push_back(std::move(*parsed));
          else throw CLI::ValidationError("--section-start", arg + " is not name=address");
      },
      "Set address of named section")
      ->option_text("SECTION=ADDRESS")
      // One argument per use, so that repeatable options do not consume the input files after them.
      ->allow_extra_args(false);
  ld->add_option("--exclude-libs", config.exclude_libs, "Exclude libraries from automatic export")
      ->option_text("lib,lib,...")
      ->delimiter(',')
      ->allow_extra_args(false);
  ld->add_option_function<std::string>(
      "--retain-symbols-file",
      [](const std::string &path) {
        config.retain_symbols = pepp::link::read_symbol_list(path);
        if (!config.retain_symbols) throw CLI::ValidationError("--retain-symbols-file", path + ": cannot read file");
      },
      "Keep only symbols listed in FILE")
      ->option_text("FILE");
  // Triggered as each is parsed, so that -R files keep their place among the other inputs.
  const auto input = [](bool just_symbols) {
    return [just_symbols](const std::vector<std::string> &paths) {
      for (const auto &path : paths)
        config.inputs.push_back({.source = std::filesystem::path(path), .just_symbols = just_symbols});
    };
  };
  ld->add_option_function<std::vector<std::string>>("-R,--just-symbols", input(true), "Just link symbols")
      ->option_text("FILE")
      ->allow_extra_args(false)
      ->trigger_on_parse();
  ld->add_option_function<std::vector<std::string>>("files", input(false), "Input object files and archives")
      ->option_text("file...")
      ->trigger_on_parse();
  ld->callback([&]() {
    flags.kind = detail::SharedFlags::Kind::TERM;
    task = [&](QObject *parent) { return new LdTask(config, parent); };
  });
}
