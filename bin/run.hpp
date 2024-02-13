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
#include "./task.hpp"
#include "elfio/elfio.hpp"

class RunTask : public Task {
  // Task interface
public:
  RunTask(int ed, std::string fname, QObject *parent);
  bool loadToElf();
  void run() override;
  void setCharOut(std::string fname);
  void setCharIn(std::string fname);
  void setMemDump(std::string fname);
  void setMaxSteps(quint64 maxSteps);
  void setBm(bool forceBm);
  void setOsIn(std::string fname);
  void setSkipLoad(bool skip);
  void setSkipDispatch(bool skip);
  void addRegisterOverride(std::string name, quint16 value);

private:
  int _ed;
  std::string _objIn;
  QSharedPointer<ELFIO::elfio> _elf = nullptr;
  std::string _charOut, _charIn, _memDump;
  quint64 _maxSteps;
  std::optional<std::string> _osIn;
  bool _skipLoad = false, _skipDispatch = false, _forceBm = false;
  QMap<std::string, quint16> _regOverrides;
};
