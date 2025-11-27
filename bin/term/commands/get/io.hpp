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
#include "../../task.hpp"

class GetIOTask : public Task {
public:
  GetIOTask(int ed, std::string ch, std::string fig, bool isFigure /*1 is figure, 0 is problem*/, int testIdx,
            QObject *parent = nullptr);
  void run() override;

private:
  int ed, testIdx;
  bool isFigure;
  std::string ch, fig;
};
