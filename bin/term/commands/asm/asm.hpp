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
#include "../../task.hpp"
#include "spdlog/logger.h"

class AsmTask : public Task {
public:
  AsmTask(int ed, std::string userFname, QObject *parent = nullptr);
  void setBm(bool forceBm);
  void setOsFname(std::string fname);
  void setErrName(std::string fname);
  void setPepoName(std::string fname);
  void setOsListingFname(std::string fname);
  void emitElfTo(std::string fname);
  void setMacroDirs(std::list<std::string> dirs);
  void run() override;

private:
  int ed;
  std::string userIn;
  bool forceBm = false;
  std::optional<std::string> osIn, peplOut, elfOut, osListOut, errOut, pepoOut;
  std::list<std::string> macroDirs;
  spdlog::logger _log{"Pepp"};
};
