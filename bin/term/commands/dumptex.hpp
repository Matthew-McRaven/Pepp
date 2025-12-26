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

#pragma once
#include "../shared.hpp"
#include "../task.hpp"

class DumpTexTask : public Task {
public:
  explicit DumpTexTask(QString dir, QObject *parent = nullptr);
  void run() override;

private:
  QString _dir;
};

void registerDumpTex(auto &app, task_factory_t &task, detail::SharedFlags &flags) {
  static std::string dirName;
  static auto tex = app.add_subcommand("tex", "Copy all \"exported\" CS6E figures into pep10tex source directory");
  auto dir = tex->add_option("directory", dirName, "Full path to pep10tex/CS6E directory");
  dir->required();
  tex->group("");
  tex->callback([&]() {
    flags.kind = detail::SharedFlags::Kind::TERM;
    task = [&](QObject *parent) { return new DumpTexTask(QString::fromStdString(dirName), parent); };
  });
}
