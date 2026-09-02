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

class AsTask : public Task {
public:
  struct Options {
    std::string elffile;
  };
  AsTask(Options &opts, QObject *parent = nullptr);
  void run() override;

private:
  Options &_opts;
};

void registerAs(auto &app, task_factory_t &task, detail::SharedFlags &flags) {
  static AsTask::Options opts;
  static auto as_clone = app.add_subcommand("as", "GNU as-compatible assembler");

  as_clone->callback([&]() {
    flags.kind = detail::SharedFlags::Kind::TERM;
    task = [&](QObject *parent) { return new AsTask(opts, parent); };
  });
}
