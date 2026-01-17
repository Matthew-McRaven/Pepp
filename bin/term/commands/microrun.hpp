/*
 * Copyright (c) 2023-2025 J. Stanley Warford, Matthew McRaven
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

class MicroRunTask : public Task {
  // Task interface
public:
  MicroRunTask(int ed, std::string fname, int busWidth, QObject *parent);
  void setUnitTests(std::string fname);
  void setErrName(std::string fname);
  void run() override;

private:
  int _ed, _busWidth;
  std::string _pecpuIn;
  std::optional<std::string> _unitTest, _errName;
};

void registerMicroRun(auto &app, task_factory_t &task, detail::SharedFlags &flags) {
  // Must initialize,
  static std::string pepcpuIn, testIn, errName;
  static int busWidth = 1;
  static auto microrunSC = app.add_subcommand("microrun", "Run micrcode programs");
  microrunSC->add_option("-s,microcode", pepcpuIn)->required()->expected(1);
  static const auto testOpt = microrunSC->add_option("-t,test", testIn)->expected(1);
  static auto errOpt =
      microrunSC->add_option("-e", errName, "File which errors will be written into. Defaults to <input>.err.txt");
  static auto busOpt = microrunSC->add_option("-w,--bus-width", busWidth, "Data bus width in bytes. Must be 1 or 2")
                           ->check(CLI::Bound(1, 2))
                           ->default_val(1);
  microrunSC->callback([&]() {
    flags.kind = detail::SharedFlags::Kind::TERM;
    task = [&](QObject *parent) {
      auto ret = new MicroRunTask(flags.edValue, pepcpuIn, busWidth, parent);
      if (*testOpt) ret->setUnitTests(testIn);
      if (*errOpt) ret->setErrName(errName);
      return ret;
    };
  });
}
