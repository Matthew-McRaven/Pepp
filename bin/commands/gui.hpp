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

#include "../shared.hpp"
#include "../task.hpp"

struct gui_args {
  std::string app_name;
};
int gui_main(gui_args);
void registerGUI(auto &app, task_factory_t &task, detail::SharedFlags &flags) {
  static auto gui = app.add_subcommand("gui", "Start Pepp GUI");
  gui->prefix_command(true);
  gui->set_help_flag();
  gui->callback([&]() { flags.isGUI = true; });
#if DEFAULT_GUI
  flags.isGUI = true;
#endif
}