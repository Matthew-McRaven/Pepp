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

#include <QtCore>
#include "../shared.hpp"
#include "../task.hpp"

class ThroughputTask : public Task {
  Q_OBJECT
public:
  ThroughputTask(QObject *parent = nullptr);
  ;
  ~ThroughputTask() = default;
  void run();
};

void registerThroughput(auto &app, task_factory_t &task, detail::SharedFlags &flags) {
  static auto instrThruSC = app.add_subcommand("mit", "Measure instruction throughput");
  instrThruSC->group("");
  instrThruSC->callback([&]() {
    flags.kind = detail::SharedFlags::TERM;
    task = [](QObject *parent) { return new ThroughputTask(parent); };
  });
}
