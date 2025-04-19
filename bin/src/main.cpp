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

#include <CLI11.hpp>
#include <QtCore>
#include <iostream>
#include "./shared.hpp"
#include "./task.hpp"
#include "commands/about.hpp"
#include "commands/asm.hpp"
#include "commands/dumpbooks.hpp"
#include "commands/get.hpp"
#include "commands/gui.hpp"
#include "commands/license.hpp"
#include "commands/ls-qrc.hpp"
#include "commands/ls.hpp"
#include "commands/readelf/readelf.hpp"
#include "commands/run.hpp"
#include "commands/selftest.hpp"
#include "commands/throughput.hpp"

#if defined(Q_OS_WASM)
const bool is_wasm = true;
#else
const bool is_wasm = false;
#endif

int main(int argc, char **argv) {
  // Get the name of the executable, and see if it ends in term.
  // If so, we should present terminal help on being called with no args.
  QFile execFile(argv[0]);
  QFileInfo execInfo(execFile);
  auto name = execInfo.baseName();
  bool default_term = name.endsWith("term", Qt::CaseInsensitive) && !is_wasm;
  CLI::App app{"Pepp", "pepp"};
  app.prefix_command(!default_term);
  app.set_help_flag("-h,--help", "Display this help message and exit.");

  auto shared_flags = detail::SharedFlags{.kind = detail::SharedFlags::Kind::DEFAULT};
  auto ed = app.add_flag("-e,--edition", shared_flags.edValue,
                         "Which edition of Computer Systems to target. "
                         "Possible values are 4, 5, and 6.")
                ->default_val(6)
                ->expected(4, 6);
  task_factory_t task = nullptr;

  registerLicense(app, task, shared_flags);
  registerAbout(app, task, shared_flags);
  registerSelfTest(app, task, shared_flags);

  registerList(app, task, shared_flags);
  registerListQRC(app, task, shared_flags);
  registerGet(app, task, shared_flags);

  registerAsm(app, task, shared_flags);
  registerRun(app, task, shared_flags);
  registerReadelf(app, task, shared_flags);
  gui_args args{.argvs = {argv[0]}};
  registerGUI(app, task, shared_flags, args);
  auto resetSettings = app.add_flag("--reset-settings", args.resetSettings, "Reset settings to default");

  // Hidden commands
  registerThroughput(app, task, shared_flags);
  registerDumpBooks(app, task, shared_flags);
  try {
    app.parse(argc, argv);
    // If kind is default, then no subcommand was called, forward all arguments.
    if (shared_flags.kind == detail::SharedFlags::Kind::DEFAULT && !default_term)
      std::transform(argv + 1, argv + argc, std::back_inserter(args.argvs), [](char *s) { return std::string(s); });
    else if (!(task || shared_flags.kind == detail::SharedFlags::Kind::GUI)) throw CLI::CallForHelp();
  } catch (const CLI::CallForHelp &e) {
    std::cout << app.help() << std::endl;
    return 0;
  } catch (const CLI::ParseError &e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }
  if (shared_flags.kind == detail::SharedFlags::Kind::GUI ||
      (shared_flags.kind == detail::SharedFlags::Kind::DEFAULT) && !default_term) {
#if INCLUDE_GUI
    return gui_main(args);
#else
    std::cerr << "GUI is not supported" << std::endl;
    return 4;
#endif
  } else {
    // TODO: Fix arg passing
    QCoreApplication a(argc, argv);
    auto taskInstance = task(&a);
    QObject::connect(taskInstance, &Task::finished, &a, QCoreApplication::exit, Qt::QueuedConnection);
    QTimer::singleShot(0, taskInstance, &Task::run);
    return a.exec();
  }
}
