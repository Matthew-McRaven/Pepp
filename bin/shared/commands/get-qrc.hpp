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

#pragma once
#include "../shared.hpp"
#include "../task.hpp"

class GetQRCTask : public Task {
public:
  GetQRCTask(QString source, QString dest, QObject *parent = nullptr);
  void run() override;

private:
  QString _src, _dst;
};

void registerGetQRC(auto &app, task_factory_t &task, detail::SharedFlags &flags) {
  static std::string source = "", dest = "";
  static auto get_qrc = app.add_subcommand("get-qrc", "Extract a single file from ");
  get_qrc->add_option("qrc-source", source, "File relative to :/ to extract");
  get_qrc->add_option("dest", dest, "File relative to PWD into which qrc-source will be copied");
  get_qrc->group("");
  get_qrc->callback([&]() {
    flags.kind = detail::SharedFlags::Kind::TERM;
    task = [&](QObject *parent) {
      return new GetQRCTask(QString::fromStdString(source), QString::fromStdString(dest), parent);
    };
  });
}
