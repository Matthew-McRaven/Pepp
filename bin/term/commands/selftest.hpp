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

class SelfTestTask : public Task {
public:
  explicit SelfTestTask(std::vector<std::string> catchArgs, QObject *parent = nullptr);
  void run() override;

private:
  std::vector<std::string> args;
};

void registerSelfTest(auto &app, task_factory_t &task, detail::SharedFlags &flags) {
  using namespace Qt::StringLiterals;
  static auto test = app.add_subcommand("selftest", "Run all integrated tests");
  // No help flag, defer to catch2's help.
  test->prefix_command(true);
  test->set_help_flag();
  test->callback([&]() {
    flags.kind = detail::SharedFlags::Kind::TERM;
    task = [&](QObject *parent) {
      auto remainingArgs = test->remaining_for_passthrough();
      std::reverse(remainingArgs.begin(), remainingArgs.end());
      // Must push executable name to argv[0], or catch arg parsing CTDs.
      auto realArgs = QCoreApplication::arguments();
      remainingArgs.insert(remainingArgs.begin(), u"%1 selftest"_s.arg(realArgs[0]).toStdString());
      return new SelfTestTask(remainingArgs);
    };
  });
}
