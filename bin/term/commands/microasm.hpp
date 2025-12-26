/*
 * Copyright (c) 2025-2026 J. Stanley Warford, Matthew McRaven
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
#include "../shared.hpp"
#include "../task.hpp"

class MicroAsmTask : public Task {
public:
  MicroAsmTask(int ed, std::string in, int busWidth, QObject *parent = nullptr);
  void setErrName(std::string fname);
  void setOutCPUName(std::string fname);
  void run() override;

private:
  int ed, busWidth = 1;
  std::string in;
  std::optional<std::string> errOut, pepcpuOut;
};

void registerMicroAsm(auto &app, task_factory_t &task, detail::SharedFlags &flags) {
  static auto microasmSC = app.add_subcommand("microasm", "Assemble and format microcode");
  static std::string in, pepcpuOut, errOut;
  static int busWidth = 1;
  static auto pepcpuOpt =
      microasmSC->add_option("-o", pepcpuOut, "File which formatted microcode will be written into.");
  static auto errOpt =
      microasmSC->add_option("-e", errOut, "File which errors will be written into. Defaults to <input>.err.txt");
  static auto busOpt = microasmSC->add_option("-w,--bus-width", busWidth, "Data bus width in bytes. Must be 1 or 2")
                           ->check(CLI::Bound(1, 2))
                           ->default_val(1);
  microasmSC->add_option("-s,source", in, "Microcode input file (pepcpu)")->required()->expected(1);
  microasmSC->callback([&]() {
    flags.kind = detail::SharedFlags::Kind::TERM;
    task = [&](QObject *parent) {
      auto ret = new MicroAsmTask(flags.edValue, in, busWidth, parent);
      if (*errOpt) ret->setErrName(errOut);
      if (*pepcpuOpt) ret->setOutCPUName(pepcpuOut);
      return ret;
    };
  });
}
