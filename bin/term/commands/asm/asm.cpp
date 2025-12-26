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
#include "asm.hpp"
#include <iostream>
#include "../../basic_lazy_sink.hpp"
#include "../../shared.hpp"
#include "enums/isa/pep10.hpp"
#include "help/builtins/figure.hpp"
#include "spdlog/sinks/stdout_sinks.h"
#include "toolchain/helpers/asmb.hpp"
#include "toolchain/helpers/assemblerregistry.hpp"
#include "toolchain/macro/registry.hpp"
#include "toolchain/pas/operations/pepp/bytes.hpp"

AsmTask::AsmTask(int ed, std::string userFname, QObject *parent) : Task(parent), ed(ed), userIn(userFname) {
  auto console_sink = std::make_shared<spdlog::sinks::stderr_sink_mt>();
  console_sink->set_level(spdlog::level::warn);
  console_sink->set_pattern("%v%");

  QString fname = QString::fromStdString(userFname);
  if (errOut) fname = QString::fromStdString(*errOut);
  else if (pepoOut) fname = QString::fromStdString(*pepoOut);
  QFileInfo base(fname);
  QString errFName = base.path() + "/" + base.completeBaseName() + ".err.txt";
  auto file_sink = std::make_shared<spdlog::sinks::basic_lazy_file_sink_mt>(errFName.toStdString(), true);
  file_sink->set_level(spdlog::level::warn);
  file_sink->set_pattern("%v");
  _log.sinks() = {console_sink, file_sink};
  _log.flush_on(spdlog::level::warn);
}

void AsmTask::setBm(bool forceBm) { this->forceBm = forceBm; }

void AsmTask::setOsFname(std::string fname) { osIn = fname; }

void AsmTask::setErrName(std::string fname) { errOut = fname; }

void AsmTask::setPepoName(std::string fname) { pepoOut = fname; }

void AsmTask::setOsListingFname(std::string fname) { osListOut = fname; }

void AsmTask::setMacroDirs(std::list<std::string> dirs) { macroDirs = dirs; }

void AsmTask::emitElfTo(std::string fname) { elfOut = fname; }

void AsmTask::run() {
  auto books = helpers::builtins_registry(false);
  auto book = helpers::book(ed, &*books);
  if (book.isNull())
    return emit finished(2);
  auto macroRegistry = helpers::registry(book, {});
  helpers::addMacros(*macroRegistry, macroDirs, "Pep/10");

  QString userContents;
  {
    QFile uIn(QString::fromStdString(userIn)); // auto-closes
    if (!uIn.exists()) {
      _log.error("Source file does not exist.\n");
      emit finished(3);
    }
    uIn.open(QIODevice::ReadOnly | QIODevice::Text);
    userContents = uIn.readAll();
  }

  // If no OS, default to full.
  QString osContents;
  if (this->forceBm) {
    auto os = book->findFigure("os", "pep10baremetal");
    osContents = os->typesafeNamedFragments()["pep"]->contents();
  } else if (!osIn || osIn->empty()) {
    auto os = book->findFigure("os", "pep10os");
    osContents = os->typesafeNamedFragments()["pep"]->contents();
  } else {
    QFile oIn(QString::fromStdString(*osIn)); // auto-closes
    if (!oIn.exists()) {
      _log.error("OS file does not exist.\n");
      emit finished(4);
    }
    oIn.open(QIODevice::ReadOnly | QIODevice::Text);
    osContents = oIn.readAll();
  }
  helpers::AsmHelper helper(macroRegistry, osContents);
  helper.setUserText(userContents);
  auto result = helper.assemble();
  if (!result) {
    _log.error("Assembly failed: ");
    for (auto &error : helper.errors()) _log.error("  {}", error.toStdString());
    return emit finished(6);
  }
  // Assembly succeded!
  if (elfOut.has_value()) {
    auto elf = helper.elf();
    elf->save(elfOut.value());
  }

  QString pepoFName;
  if (pepoOut) {
    pepoFName = QString::fromStdString(*pepoOut);
  } else {
    QFileInfo pepo(QString::fromStdString(userIn));
    pepoFName = pepo.path() + "/" + pepo.completeBaseName() + ".pepo";
  }
  auto userBytes = helper.bytes(false);
  auto userBytesStr = pas::ops::pepp::bytesToObject(userBytes);
  QFile pepoF(pepoFName);
  if (pepoF.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
    QTextStream(&pepoF) << userBytesStr << "\n";
  } else _log.error("Failed to open object code for writing:  {}", pepoFName.toStdString());

  try {
    auto lines = helper.listing(false);
    QFileInfo pepl(pepoFName);
    QString peplFName = pepl.path() + "/" + pepl.completeBaseName() + ".pepl";
    QFile peplF(peplFName);
    if (peplF.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
      auto ts = QTextStream(&peplF);
      for (auto &line : lines)
        ts << line << "\n";
    } else _log.error("Failed to open listing for writing:  {}", peplFName.toStdString());
  } catch (std::exception &e) {
    _log.error("Failed to generate user listing due to internal bug.");
  }
  if (osListOut) {
    try {
      auto lines = helper.listing(true);
      QFile peplF(QString::fromStdString(*osListOut));
      if (peplF.open(QFile::OpenModeFlag::WriteOnly)) {
        auto ts = QTextStream(&peplF);
        for (auto &line : lines)
          ts << line << "\n";
        peplF.close();
      } else _log.error("Failed to open OS listing for writing:  {}", *osListOut);
    } catch (std::exception &e) {
      _log.error("Failed to generate os listing due to internal bug.");
    }
  }

  return emit finished(0);
}
