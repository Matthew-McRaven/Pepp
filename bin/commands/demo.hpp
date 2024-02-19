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
#include "gui.hpp"

// Don't include headers that include GUI components unless we're in GUI mode.
#if INCLUDE_GUI
#include "../demo/asm/main.hpp"
#include "../demo/fig-view/main.hpp"
#endif

void registerDemo(auto &app, task_factory_t &task, detail::SharedFlags &flags, gui_args &args) {
  static auto demo = app.add_subcommand("demo", "Start a Pepp GUI demo");
  demo->set_help_flag();
  demo->callback([&]() { flags.kind = detail::SharedFlags::GUI; });
  static auto asmDemo = demo->add_subcommand("asm", "Start the assembler demo");
  asmDemo->set_help_flag();
  static auto figDemo = demo->add_subcommand("fig", "Start the figure viewer demo");
  figDemo->set_help_flag();
#if INCLUDE_GUI
  asmDemo->callback([&]() {
    args.extra_init = &initializeAsm;
    args.QMLEntry = asmQMLMain;
  });
  figDemo->callback([&]() {
    args.extra_init = &initializeFigView;
    args.QMLEntry = figviewQMLMain;
  });
#endif
}
