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
#include "../shared.hpp"
#include "../task.hpp"
#include "elfio/elfio.hpp"
#include "spdlog/logger.h"

class RVEmuTask : public Task {
  // Task interface
public:
  struct Arguments {
    bool verbose = false;
    bool quit = false;
    bool accurate = false;
    bool instr_trace = true;
    bool debug = false;
    bool silent = false;
    bool from_start = false;
    bool sandbox = true;
    bool execute_only = false;
    bool ignore_text = false;
    uint64_t fuel = 30'000'000'000ULL; // Default: Timeout after ~30bn instructions
    static constexpr uint64_t MAX_MEMORY = uint64_t(4000) << 20;
    uint64_t max_memory = MAX_MEMORY;
    std::vector<std::string> allowed_files = {};
    std::vector<std::string> prog_args = {};
    std::string call_function = "";
  };
  RVEmuTask(const Arguments args, std::string fname, QObject *parent);
  void run() override;

private:
  const Arguments args;
  const std::string fname;
  spdlog::logger _log{"Pepp"};
};

void register_rvemu(CLI::App &app, task_factory_t &task, detail::SharedFlags &flags);
