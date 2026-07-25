/*
 * Copyright (c) 2024-2026 J. Stanley Warford, Matthew McRaven
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

#include "./interactive.hpp"
#include <catch.hpp>
#include <iostream>
#include "core/interactive_test/hostobjs/vocab.hpp"
#include "core/interactive_test/interp.hpp"
#include "core/interactive_test/vocab/core_words.hpp"

InteractiveTask::InteractiveTask(QObject *parent) : Task(parent) {}

void InteractiveTask::run() {
  std::cout << "Interactive mode. Type something and press enter (Ctrl+D to exit):" << std::endl;
  std::string input;
  Interpreter p;
  register_common_words(&p);
  register_devicemgmt_words(&p);
  std::cout.flush();
  std::cerr.flush();

  p.input_source = std::make_unique<StdinInput>();
  p.run();
  return emit finished(0);
}