
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

#include "selftest.hpp"
#include <catch.hpp>
#include <utility>
SelfTestTask::SelfTestTask(std::vector<std::string> catchArgs, QObject *parent)
    : Task(parent), args(std::move(catchArgs)) {}
void SelfTestTask::run() {
  Catch::Session session; // There must be exactly one instance
  std::vector<const char *> argv;
  argv.reserve(args.size());
  std::transform(args.begin(), args.end(), std::back_inserter(argv), [](const std::string &s) { return s.c_str(); });
  session.applyCommandLine(static_cast<int>(argv.size()), argv.data());
  return emit finished(session.run());
}
