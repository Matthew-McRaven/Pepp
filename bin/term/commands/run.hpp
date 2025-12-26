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
#include <CLI11.hpp>
#include "../shared.hpp"
#include "../task.hpp"
#include "elfio/elfio.hpp"
#include "spdlog/logger.h"

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
  void addRegisterOverride(std::string name, quint16 value);

private:
  int _ed;
  std::string _objIn;
  QSharedPointer<ELFIO::elfio> _elf = nullptr;
  std::string _charOut, _charIn, _memDump;
  quint64 _maxSteps;
  std::optional<std::string> _osIn;
  bool _forceBm = false;
  QMap<std::string, quint16> _regOverrides;
  spdlog::logger _log{"Pepp"};
};

void registerRun(auto &app, task_factory_t &task, detail::SharedFlags &flags) {
  // Must initialize,
  static bool bm = false;
  static std::string objIn, charIn, charOut, memDump, osIn;
  static uint64_t maxSteps;
  static std::map<std::string, quint64> regOverrides;
  static CLI::Option *bmRunOpt = nullptr;

  static auto runSC = app.add_subcommand("run", "Run ISA3 programs");
  static auto charInOpt = runSC->add_option("-i,--charIn", charIn,
                                            "File whose contents are to be buffered behind charIn. The value `-` "
                                            "will cause charIn to be taken from stdin. When using `-`, failure to "
                                            "provide stdin will cause program to freeze.");
  runSC
      ->add_option("-o,--charOut", charOut,
                   "File to which the contents of charOut will be written. "
                   "The value `-` specifies stdout")
      ->default_val("-");
  static auto memDumpOpt =
      runSC->add_option("--mem-dump", memDump, "File to which post-simulation memory-dump will be written.");
  runSC->add_option("-s,obj", objIn)->required()->expected(1);
  runSC
      ->add_option("--max,-m", maxSteps,
                   "Maximum number of instructions that will be executed "
                   "before terminating simulator.")
      ->default_val(125'000);
  static auto osInOpt = runSC->add_option("--os", osIn, "File from which os will be read.");
  if (flags.edValue == 6)
    bmRunOpt = runSC->add_flag("--bm", bm, "Use bare metal OS.")->excludes(osInOpt);
  static auto regOverrideOpt = runSC->add_option("--reg", regOverrides)->group("");
  runSC->callback([&]() {
    flags.kind = detail::SharedFlags::Kind::TERM;
    task = [&](QObject *parent) {
      auto ret = new RunTask(flags.edValue, objIn, parent);
      if (*charInOpt)
        ret->setCharIn(charIn);
      ret->setCharOut(charOut);
      if (*memDumpOpt)
        ret->setMemDump(memDump);
      if (bmRunOpt && *bmRunOpt)
        ret->setBm(bm);
      else if (*osInOpt)
        ret->setOsIn(osIn);
      ret->setMaxSteps(maxSteps);
      for (auto &reg : regOverrides)
        ret->addRegisterOverride(reg.first, reg.second);
      return ret;
    };
  });
}
