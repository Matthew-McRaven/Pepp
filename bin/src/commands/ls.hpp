/*
 * Copyright (c) 2023-2024 J. Stanley Warford, Matthew McRaven
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
#include "../shared.hpp"
#include "../task.hpp"

class ListTask : public Task {
public:
  ListTask(int ed, QObject *parent = nullptr);
  void run() override;
  bool showFigs = true, showProbs = false, showMacros = false, showTestCount = false;

private:
  int ed;
};

void registerList(auto &app, task_factory_t &task, detail::SharedFlags &flags) {
  static auto list = app.add_subcommand("ls", "Produce list of figures and macros");
  static bool showFigs = true, showProbs = false, showMacros = false, showTestCount = false;
  static auto optFigs =
      list->add_flag(" --show-figures,!--no-show-figures", showFigs, "List figures")->default_val(true);
  static auto optOpt =
      list->add_flag("--show-problems,!--no-show-problems", showProbs, "List problems")->default_val(false);
  static auto optMacros =
      list->add_flag("--show-macros,!--no-show-macros", showMacros, "List macros")->default_val(false);
  static auto optTestCount =
      list->add_flag("--show-test-count,!--no-test-count", showTestCount, "List test count")->default_val(false);
  list->callback([&]() {
    flags.kind = detail::SharedFlags::Kind::TERM;
    task = [&](QObject *parent) {
      auto ret = new ListTask(flags.edValue, parent);
      ret->showFigs = showFigs;
      ret->showProbs = showProbs;
      ret->showMacros = showMacros;
      ret->showTestCount = showTestCount;
      return ret;
    };
  });
}
