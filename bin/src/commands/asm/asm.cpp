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
#include "asm.hpp"
#include <iostream>
#include "../../shared.hpp"
#include "enums/isa/pep10.hpp"
#include "help/builtins/figure.hpp"
#include "toolchain/helpers/asmb.hpp"
#include "toolchain/helpers/assemblerregistry.hpp"
#include "toolchain/macro/registry.hpp"
#include "toolchain/pas/operations/pepp/bytes.hpp"

AsmTask::AsmTask(int ed, std::string userFname, QObject *parent) : Task(parent), ed(ed), userIn(userFname) {}

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
      std::cerr << "Source file does not exist.\n";
      emit finished(3);
    }
    uIn.open(QIODevice::ReadOnly | QIODevice::Text);
    userContents = uIn.readAll();
  }

  // If no OS, default to full.
  QString osContents;
  if (this->forceBm) {
    auto os = book->findFigure("os", "pep10baremetal");
    osContents = os->typesafeNamedElements()["pep"]->contents();
  } else if (!osIn || osIn->empty()) {
    auto os = book->findFigure("os", "pep10os");
    osContents = os->typesafeNamedElements()["pep"]->contents();
  } else {
    QFile oIn(QString::fromStdString(*osIn)); // auto-closes
    if (!oIn.exists()) {
      std::cerr << "OS file does not exist.\n";
      emit finished(4);
    }
    oIn.open(QIODevice::ReadOnly | QIODevice::Text);
    osContents = oIn.readAll();
  }
  helpers::AsmHelper helper(macroRegistry, osContents);
  helper.setUserText(userContents);
  auto result = helper.assemble();
  if (!result) {
    std::cerr << "Assembly failed, check error log" << std::endl;
    QString errFName;
    if (errOut) {
      errFName = QString::fromStdString(*errOut);
    } else {
      QFileInfo err(QString::fromStdString(userIn));
      errFName = err.path() + "/" + err.completeBaseName() + ".err.txt";
    }
    QFile errF(errFName);

    if (errF.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
      auto ts = QTextStream(&errF);
      for (auto &error : helper.errors())
        ts << error;
    } else {
      std::cerr << "Failed to open error log for writing: " << errFName.toStdString() << std::endl;
    }
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
  } else
    std::cerr << "Failed to open object code for writing: " << pepoFName.toStdString() << std::endl;

  try {
    auto lines = helper.listing(false);
    QFileInfo pepl(pepoFName);
    QString peplFName = pepl.path() + "/" + pepl.completeBaseName() + ".pepl";
    QFile peplF(peplFName);
    if (peplF.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
      auto ts = QTextStream(&peplF);
      for (auto &line : lines)
        ts << line << "\n";
    } else
      std::cerr << "Failed to open listing for writing: " << peplFName.toStdString() << std::endl;
  } catch (std::exception &e) {
    std::cerr << "Failed to generate user listing due to internal bug.\n";
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
      } else
        std::cerr << "Failed to open listing for writing: " << *osListOut << std::endl;
    } catch (std::exception &e) {
      std::cerr << "Failed to generate listing due to internal bug.\n";
    }
  }

  return emit finished(0);
}
