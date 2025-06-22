/*
 * Copyright (c) 2024 J. Stanley Warford, Matthew McRaven
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
#include <string>
#include "../shared.hpp"
#include "../task.hpp"
#include "get/fig.hpp"
#include "get/io.hpp"
#include "get/macro.hpp"

void registerGet(auto &app, task_factory_t &task, detail::SharedFlags &flags) {
  static auto get = app.add_subcommand("get", "Fetch the body of a figure or macro");
  static auto get_selector = get->add_option_group("figure_or_macro")->required();
  static auto get_figure = get_selector->add_option_group("[--ch]");
  static std::string ch, fig, macro, type, prob;
  static int inputValue = 0;
  static auto chOpt = get_figure->add_option("--ch", ch, "")->required();
  static auto get_item = get_figure->add_option_group("[--fig, --prob]");
  static auto figOpt = get_item->add_option("--fig", fig, "");
  static auto probOpt = get_item->add_option("--prob", prob, "");
  static auto get_type = get_figure->add_option_group("[--input, --type]");
  static auto inputOpt = get_type->add_option("--input", inputValue, "");
  static auto typeOpt = get_type->add_option("--type", type, "")->default_val("pep");
  static auto macroOpt = get_selector->add_option("--macro", macro, "");
  get_selector->require_option(1);
  get_item->require_option(1);
  get_type->require_option(1);
  get->callback([&]() {
    flags.kind = detail::SharedFlags::Kind::TERM;
    if (chOpt->count() > 0)
      task = [&](QObject *parent) {
        Task *ret = nullptr;
        if (*inputOpt) ret = new GetIOTask(flags.edValue, ch, fig, true, inputValue, parent);
        else if (*figOpt) ret = new GetFigTask(flags.edValue, ch, fig, type, true, parent);
        else if (*probOpt) ret = new GetFigTask(flags.edValue, ch, prob, type, false, parent);
        return ret;
      };
    else if (macroOpt->count() > 0)
      task = [&](QObject *parent) { return new GetMacroTask(flags.edValue, macro, parent); };
  });
}
