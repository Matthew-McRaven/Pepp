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
#include <QDebug>
#include <QtCore>
#include <chrono>
#include <iostream>
#include <thread>
#include "./shared.hpp"
#include "./task.hpp"
#include "commands/about.hpp"
#include "commands/asm.hpp"
#include "commands/get.hpp"
#include "commands/license.hpp"
#include "commands/ls.hpp"
#include "commands/run.hpp"
#include "commands/throughput.hpp"

int main(int argc, char **argv) {

  CLI::App app{"Pepp", "pepp"};
  app.set_help_flag("-h,--help", "Display this help message and exit.");

  auto shared_flags = detail::SharedFlags{};
  auto ed = app.add_flag("-e,--edition", shared_flags.edValue,
                         "Which edition of Computer Systems to target. "
                         "Possible values are 4, 5, and 6.")
                ->default_val(6)
                ->expected(4, 6);
  task_factory_t task = nullptr;

  registerLicense(app, task, shared_flags);
  registerAbout(app, task, shared_flags);

  registerList(app, task, shared_flags);
  registerGet(app, task, shared_flags);

  registerAsm(app, task, shared_flags);
  registerRun(app, task, shared_flags);

  // Hidden commands
  registerThroughput(app, task, shared_flags);
  try {
    app.parse(argc, argv);
    if (!task)
      throw CLI::CallForHelp();
  } catch (const CLI::CallForHelp &e) {
    std::cout << app.help() << std::endl;
    return 0;
  } catch (const CLI::ParseError &e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }
  QCoreApplication a(argc, argv);
  auto taskInstance = task(&a);
  QObject::connect(taskInstance, &Task::finished, &a, QCoreApplication::exit, Qt::QueuedConnection);
  QTimer::singleShot(0, taskInstance, &Task::run);
  return a.exec();
}

Task::Task(QObject *parent) : QObject(parent) {}
