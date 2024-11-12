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

#pragma once

#include <CLI11.hpp>
#include <string>
#include "../shared.hpp"
#include "./asm/asm.hpp"

void registerAsm(auto &app, task_factory_t &task, detail::SharedFlags &flags) {
  static auto asmSC = app.add_subcommand("asm", "Assemble stuff");
  static bool bm;
  static std::string userName, osListing, pepoOut, errOut;
  static std::optional<std::string> osName = std::nullopt, elfName = std::nullopt;
  static std::list<std::string> macroDirs;
  static CLI::Option *bmAsmOpt = nullptr;
  static auto osOpt = asmSC->add_option("--os", osName);
  if (flags.edValue == 6)
    bmAsmOpt = asmSC->add_flag("--bm", bm, "Use bare metal OS.")->excludes(osOpt);
  static auto elfOpt = asmSC->add_option("--elf", elfName);
  static auto pepoOpt = asmSC->add_option("-o", pepoOut);
  static auto errOpt = asmSC->add_option("-e", errOut);
  static auto osListingOpt = asmSC->add_option("--os-listing", osListing);
  static auto macroDirOpts = asmSC->add_option("--md,--macro-dir", macroDirs);

  asmSC->add_option("-s,user", userName)->required()->expected(1);
  asmSC->callback([&]() {
    flags.kind = detail::SharedFlags::Kind::TERM;
    task = [&](QObject *parent) {
      auto ret = new AsmTask(flags.edValue, userName, parent);
      if (bmAsmOpt && *bmAsmOpt)
        ret->setBm(bm);
      else if (*osOpt)
        ret->setOsFname(*osName);
      if (*elfOpt)
        ret->emitElfTo(*elfName);
      if (*osListingOpt)
        ret->setOsListingFname(osListing);
      if (*errOpt)
        ret->setErrName(errOut);
      if (*pepoOpt)
        ret->setPepoName(pepoOut);
      if (*macroDirOpts)
        ret->setMacroDirs(macroDirs);
      return ret;
    };
  });
}
