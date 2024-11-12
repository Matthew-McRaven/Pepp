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

#include "../shared.hpp"
#include "../task.hpp"

struct gui_args {
  std::vector<std::string> argvs;
#if INCLUDE_GUI
  QUrl QMLEntry{};              // If empty, pick "GUI" default.
#endif
};

int gui_main(const gui_args &);

void registerGUI(auto &app, task_factory_t &task, detail::SharedFlags &flags, gui_args &gui_args) {
  static auto gui = app.add_subcommand("gui", "Start Pepp GUI");
  gui->prefix_command(true);
  gui->set_help_flag();
  gui->callback([&]() {
    flags.kind = detail::SharedFlags::Kind::GUI;
    gui_args.argvs = gui->remaining_for_passthrough();
    std::reverse(gui_args.argvs.begin(), gui_args.argvs.end());
  });
}
