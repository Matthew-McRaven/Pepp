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

#include "run.hpp"
#include "../basic_lazy_sink.hpp"
#include "../shared.hpp"
#include "core/libs/bitmanip/strings.hpp"
#include "help/builtins/figure.hpp"
#include "sim3/cores/pep/traced_helpers.hpp"
#include "sim3/cores/pep/traced_pep10_isa3.hpp"
#include "sim3/subsystems/bus/simple.hpp"
#include "sim3/subsystems/ram/broadcast/mmi.hpp"
#include "sim3/subsystems/ram/broadcast/mmo.hpp"
#include "sim3/systems/traced_pep_isa3_system.hpp"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_sinks.h"
#include "spdlog/spdlog.h"
#include "toolchain/helpers/asmb.hpp"
#include "toolchain/helpers/assemblerregistry.hpp"
#include "toolchain/link/mmio.hpp"

auto gs = sim::api2::memory::Operation{
    .type = sim::api2::memory::Operation::Type::Application,
    .kind = sim::api2::memory::Operation::Kind::data,
};

RunTask::RunTask(int ed, std::string fname, QObject *parent) : Task(parent), _ed(ed), _objIn(fname) {
  auto console_sink = std::make_shared<spdlog::sinks::stderr_sink_mt>();
  console_sink->set_level(spdlog::level::warn);
  console_sink->set_pattern("%v%");

  QFileInfo pepo(QString::fromStdString(fname));
  QString errFName = pepo.path() + "/" + pepo.completeBaseName() + ".err.txt";
  auto file_sink = std::make_shared<spdlog::sinks::basic_lazy_file_sink_mt>(errFName.toStdString(), true);
  file_sink->set_level(spdlog::level::warn);
  file_sink->set_pattern("%v");
  _log.sinks() = {console_sink, file_sink};
  _log.flush_on(spdlog::level::warn);
}

bool RunTask::loadToElf() {
  auto ret = QSharedPointer<ELFIO::elfio>::create();
  if (ret->load(_objIn)) {
    _elf = ret;
    return true;
  }
  auto books = helpers::builtins_registry(false);
  auto book = helpers::book(_ed, &*books);
  if (book.isNull())
    return false;
  QString osContents;
  if (_forceBm && _ed == 6) {
    auto os = book->findFigure("os", "pep10baremetal");
    osContents = os->typesafeNamedFragments()["pep"]->contents();
  } else if (!_osIn.has_value()) {
    auto os = book->findFigure("os", "pep10os");
    osContents = os->typesafeNamedFragments()["pep"]->contents();
  } else {
    QFile oIn(QString::fromStdString(*_osIn)); // auto-closes
    oIn.open(QIODevice::ReadOnly | QIODevice::Text);
    osContents = oIn.readAll();
  }
  auto macroRegistry = helpers::registry(book, {});
  helpers::AsmHelper helper(macroRegistry, osContents);
  auto result = helper.assemble();
  if (!result) {
    _log.error("OS assembly failed");
    return false;
  }
  QFile objF(QString::fromStdString(_objIn));
  if (!objF.open(QIODevice::ReadOnly | QIODevice::Text)) {
    const QFileInfo fi(objF);
    _log.error("Failed to open object code");
    _log.error("  path: {}\n  absPath: {}", _objIn, fi.absoluteFilePath().toStdString());
    _log.error("  erno: {}\n  errpr: {}", (int)objF.error(), objF.errorString().toStdString());
    return false;
  }
  auto objText = objF.readAll().toStdString();
  auto bytes = bits::asciiHexToByte({objText.data(), objText.size()});
  if (!bytes)
    return false;
  _elf = helper.elf(&bytes.value());
  return true;
}

void RunTask::run() {
  using namespace Qt::StringLiterals;
  if (!loadToElf())
    return emit finished(1);
  auto system = targets::isa::systemFromElf(*_elf, true);
  system->init();

  // Perform any requested register overrides.
  for (auto [reg, val] : _regOverrides.asKeyValueRange()) {
    int reg_val = 0;
    switch (_ed) {
    case 6: {
      auto regEnu = ::isa::Pep10::parseRegister(reg);
      if (regEnu == isa::Pep10::Register::INVALID) {
        static const char *const e = "Invalid register";
        _log.critical(e);
        throw std::logic_error(e);
      }
      auto cpu = static_cast<targets::pep10::isa::CPU *>(system->cpu());
      targets::isa::writeRegister<isa::Pep10>(cpu->regs(), regEnu, val, gs);
      break;
    }
    default:
      static const char *const e = "Unhandled book";
      _log.critical(e);
      throw std::logic_error(e);
    }
  }

  auto pwrOff = system->output("pwrOff");
  auto endpoint = pwrOff->endpoint();
  bool fail = false;
  if (auto charIn = system->input("charIn"); !_charIn.empty() && charIn) {
    auto charInEndpoint = charIn->endpoint();
    QString buffer;

    if (_charIn == "-") {
      QTextStream in(stdin, QIODevice::ReadOnly | QIODevice::Text);
      while (!in.atEnd())
        charInEndpoint->append_value(in.read(1).toUtf8()[0]);
    } else {
      QFile f(QString::fromStdString(_charIn));
      f.open(QIODevice::ReadOnly | QIODevice::Text);
      QByteArray buffer = f.readAll();
      for (int it = 0; it < buffer.size(); it++)
        charInEndpoint->append_value(buffer[it]);
    }
  }

  auto printReg = [&](isa::Pep10::Register reg) {
    quint16 tmp = 0;
    auto cpu = static_cast<targets::pep10::isa::CPU *>(system->cpu());
    targets::isa::readRegister<isa::Pep10>(cpu->regs(), reg, tmp, gs);
    auto regName = isa::Pep10::string(reg);
    std::cout << u"%1=%2"_s.arg(regName).arg(QString::number(tmp, 16), 4, '0').toStdString() << " ";
  };
  bool noMMI = false;

  try {
    while (system->currentTick() < _maxSteps && !endpoint->next_value().has_value())
      system->tick(sim::api2::Scheduler::Mode::Jump);
  } catch (const sim::api2::memory::Error &e) {
    if (e.type() == sim::api2::memory::Error::Type::NeedsMMI) {
      noMMI = true;
    } else _log.error("Memory error: {}", e.what());
  } catch (const targets::isa::IllegalOpcode &e) {
    _log.error("Program attempted to execute an illegal opcode.\nTerminating");
    return emit finished(12);
  }

  if (noMMI) _log.error("Program requested data from charIn, but no data is present.");
  if (system->currentTick() >= _maxSteps) {
    _log.error("Exceeded max number of steps. Possible infinite loop");
  }
  if (auto charOut = system->output("charOut"); !_charOut.empty() && charOut) {
    auto charOutEndpoint = charOut->endpoint();
    charOutEndpoint->set_to_head();
    auto writeOut = [&](QTextStream &outF) {
      for (auto next = charOutEndpoint->next_value(); next.has_value(); next = charOutEndpoint->next_value()) {
        outF << char(*next);
      }
    };

    if (_charOut == "-") {
      QTextStream out(stdout, QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
      writeOut(out);
      // If writing to terminal, ensure that there exists a \n.
      out << "\n";
      out.flush();

    } else {
      QFile f(QString::fromStdString(_charOut));
      f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
      QTextStream out(&f);
      writeOut(out);
      out.flush();
    }
  }

  if (!_memDump.empty()) {
    QVector<quint8> dump(0x1'00'00);
    system->bus()->dump({dump.data(), std::size_t(dump.size())});
    QFile memDump(QString::fromStdString(_memDump));
    if (memDump.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
      memDump.write(reinterpret_cast<const char *>(dump.constData()), dump.size());
      memDump.close();
    }
  }
  emit finished(0);
}

void RunTask::setCharOut(std::string fname) { this->_charOut = fname; }

void RunTask::setCharIn(std::string fname) { this->_charIn = fname; }

void RunTask::setMemDump(std::string fname) { _memDump = fname; }

void RunTask::setMaxSteps(quint64 maxSteps) { this->_maxSteps = maxSteps; }

void RunTask::setBm(bool forceBm) { _forceBm = forceBm; }

void RunTask::setOsIn(std::string fname) { _osIn = fname; }

void RunTask::addRegisterOverride(std::string name, quint16 value) { _regOverrides[name] = value; }
