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

#include <CLI11.hpp>
#include <QtCore>
#include <iostream>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>
#include "./shared.hpp"
#include "./task.hpp"
#include "commands/about.hpp"
#include "commands/asm.hpp"
#include "commands/binutils/addr2line.hpp"
#include "commands/binutils/readelf.hpp"
#include "commands/dumpbooks.hpp"
#include "commands/dumptex.hpp"
#include "commands/get-qrc.hpp"
#include "commands/get.hpp"
#include "commands/license.hpp"
#include "commands/ls-imgfmt.hpp"
#include "commands/ls-qrc.hpp"
#include "commands/ls.hpp"
#include "commands/microasm.hpp"
#include "commands/microrun.hpp"
#include "commands/run.hpp"
#include "commands/rvemu.hpp"
#include "commands/selftest.hpp"
#include "commands/throughput.hpp"

int main(int argc, char **argv) {
  // Set up some useful loggers
  auto sink = std::make_shared<spdlog::sinks::stdout_sink_mt>();
  auto create = [&](const char *name) {
    auto logger = std::make_shared<spdlog::logger>(name, sink);
    spdlog::register_logger(logger);
    return logger;
  };
  auto logger_debugger = create("debugger");
  auto logger_stack_debugger = create("debugger::stack");
  logger_debugger->set_level(spdlog::level::warn);
#if defined(SPDLOG_ACTIVE_LEVEL)
  spdlog::set_level((spdlog::level::level_enum)SPDLOG_ACTIVE_LEVEL);
#endif

  // Get the name of the executable, and see if it ends in term.
  // If so, we should present terminal help on being called with no args.
  QFile execFile(argv[0]);
  QFileInfo execInfo(execFile);
  auto name = execInfo.baseName();
  auto name_as_stdstr = name.toStdString();
  bool is_pepp_term = name.endsWith("term", Qt::CaseInsensitive);
  CLI::App app{"Pepp", "pepp"};
  app.prefix_command(is_pepp_term);
  app.set_help_flag("-h,--help", "Display this help message and exit.");

  auto shared_flags = detail::SharedFlags{.kind = detail::SharedFlags::Kind::DEFAULT};
  auto ed = app.add_option("-e,--edition", shared_flags.edValue,
                           "Which edition of Computer Systems to target. "
                           "Possible values are 4, 5, and 6.")
                ->default_val(6)
                ->check(CLI::Bound(4, 6));
  task_factory_t task = nullptr;

  registerLicense(app, task, shared_flags);
  registerAbout(app, task, shared_flags);
  registerSelfTest(app, task, shared_flags);

  registerList(app, task, shared_flags);
  registerListQRC(app, task, shared_flags);
  registerGetQRC(app, task, shared_flags);
  registerListImageFormats(app, task, shared_flags);
  registerGet(app, task, shared_flags);

  registerAsm(app, task, shared_flags);
  registerMicroAsm(app, task, shared_flags);
  registerRun(app, task, shared_flags);
  registerMicroRun(app, task, shared_flags);
  // binutils-like programs
  registerReadelf(app, task, shared_flags);
  registerAddr2Line(app, task, shared_flags);
  // qemu-like programs
  register_rvemu(app, task, shared_flags);

  // Hidden commands
  registerThroughput(app, task, shared_flags);
  registerDumpBooks(app, task, shared_flags);
  registerDumpTex(app, task, shared_flags);

  auto modified_args = std::vector<const char *>();
  for (int it = 0; it < argc; ++it) modified_args.push_back(argv[it]);
  // If being called via symlink, use the symlink name as the command name
  if (!is_pepp_term) modified_args.insert(modified_args.cbegin() + 1, name_as_stdstr.c_str());

  try {
    app.parse(modified_args.size(), modified_args.data());
    // If no subcommand was selected (and task is therfore nullptr), we must exit before someone tries to use task.
    if (task == nullptr) throw CLI::CallForHelp();
  } catch (const CLI::CallForHelp &e) {
    std::cout << app.help() << std::endl;
    return 0;
  } catch (const CLI::ParseError &e) {
    std::cerr << e.what() << std::endl;
    std::cout << app.help() << std::endl;
    return 1;
  }

  // TODO: Fix arg passing
  QCoreApplication a(argc, argv);
  auto taskInstance = task(&a);
  QObject::connect(taskInstance, &Task::finished, &a, QCoreApplication::exit, Qt::QueuedConnection);
  QTimer::singleShot(0, taskInstance, &Task::run);
  return a.exec();
}
