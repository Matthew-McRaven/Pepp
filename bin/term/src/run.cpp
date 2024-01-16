/*
 * Copyright (c) 2023 J. Stanley Warford, Matthew McRaven
 *
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

#include "./run.hpp"
#include "./shared.hpp"
#include "bits/strings.hpp"
#include "help/builtins/figure.hpp"
#include "obj/mmio.hpp"
#include "sim/device/broadcast/mmi.hpp"
#include "sim/device/broadcast/mmo.hpp"
#include "sim/device/simple_bus.hpp"
#include "targets/pep10/isa3/cpu.hpp"
#include "targets/pep10/isa3/helpers.hpp"
#include "targets/pep10/isa3/system.hpp"

static const auto gs = sim::api::memory::Operation{
    .speculative = false,
    .kind = sim::api::memory::Operation::Kind::data,
    .effectful = false,
};

RunTask::RunTask(int ed, std::string fname, QObject *parent)
    : Task(parent), _ed(ed), _objIn(fname) {}

bool RunTask::loadToElf() {
  auto ret = QSharedPointer<ELFIO::elfio>::create();
  if (ret->load(_objIn)) {
    _elf = ret;
    return true;
  }
  auto book = detail::book(_ed);
  if (book.isNull())
    return false;
  QString osContents;
  if (_forceBm && _ed == 6) {
    auto os = book->findFigure("os", "pep10baremetal");
    osContents = os->typesafeElements()["pep"]->contents;
  } else if (!_osIn.has_value()) {
    auto os = book->findFigure("os", "pep10os");
    osContents = os->typesafeElements()["pep"]->contents;
  } else {
    QFile oIn(QString::fromStdString(*_osIn)); // auto-closes
    oIn.open(QIODevice::ReadOnly | QIODevice::Text);
    osContents = oIn.readAll();
  }
  auto macroRegistry = detail::registry(book, {});
  detail::AsmHelper helper(macroRegistry, osContents);
  auto result = helper.assemble();
  if (!result) {
    std::cerr << "OS assembly failed" << std::endl;
    return false;
  }
  QFile objF(QString::fromStdString(_objIn));
  if (!objF.open(QIODevice::ReadOnly | QIODevice::Text)) {
    std::cerr << "Failed to open object code: " << _objIn << std::endl;
    return false;
  }
  auto objText = objF.readAll().toStdString();
  auto bytes = bits::asciiHexToByte({objText.data(), objText.size()});
  if (!bytes)
    return false;
  _elf = helper.elf(*bytes);
  // Need to reload to properly compute segment addresses. Store in temp
  // directory to prevent clobbering local file contents.
  {
    QTemporaryDir dir;
    if (!dir.isValid())
      return false;
    auto path = dir.filePath("tmp.elf").toStdString();
    _elf->save(path);
    _elf->load(path);
  }
  return true;
}

void RunTask::run() {
  if (!loadToElf())
    return emit finished(1);
  bool hasBootFlag = (obj::getBootFlagsAddress(*_elf).has_value());
  // Skip loading logic if there flag is set or there are no boot flags.
  bool skipLoad = _skipLoad | !hasBootFlag;
  auto system = targets::pep10::isa::systemFromElf(*_elf, skipLoad);
  system->init();
  // Skip dispatching logic if flag is set and there are boot flags.
  system->setBootFlags(!skipLoad, !_skipDispatch);

  // Perform any requested register overrides.
  for (auto [reg, val] : _regOverrides.asKeyValueRange()) {
    QMetaEnum enu;
    switch (_ed) {
    case 6:
      enu = QMetaEnum::fromType<::isa::Pep10::Register>();
      break;
    default:
      throw std::logic_error("Unhandled book");
    }
    bool ok = true;
    // Always compare in caps
    auto transformed = QString::fromStdString(reg).toUpper().toStdString();
    auto regEnu = enu.keyToValue(transformed.c_str(), &ok);
    if (!ok)
      throw std::logic_error("Invalid register");
    targets::pep10::isa::writeRegister(
        system->cpu()->regs(), static_cast<isa::Pep10::Register>(regEnu), val,
        gs);
  }

  auto pwrOff = system->output("pwrOff");
  auto endpoint = pwrOff->endpoint();
  bool fail = false;
  if (auto charIn = system->input("charIn"); !_charIn.empty() && charIn) {
    auto charInEndpoint = charIn->endpoint();
    QString buffer;

    if (_charIn == "-") {
        QTextStream in(stdin, QIODevice::ReadOnly | QIODevice::Text);
        while (!in.atEnd()) charInEndpoint->append_value(in.read(1).toUtf8()[0]);
    } else {
    QFile f(QString::fromStdString(_charIn));
    f.open(QIODevice::ReadOnly | QIODevice::Text);
    QByteArray buffer = f.readAll();
    for (int it = 0; it < buffer.size(); it++) charInEndpoint->append_value(buffer[it]);
    }
  }
  auto printReg = [&](isa::Pep10::Register reg) {
    quint16 tmp = 0;
    targets::pep10::isa::readRegister(system->cpu()->regs(), reg, tmp, gs);
    auto regName =
        QMetaEnum::fromType<isa::detail::pep10::Register>().valueToKey(
            (int)reg);
    std::cout << u"%1=%2"_qs.arg(regName)
                     .arg(QString::number(tmp, 16), 4, '0')
                     .toStdString()
              << " ";
  };
  bool noMMI = false;
  while (system->currentTick() < _maxSteps &&
         !endpoint->next_value().has_value()) {
    auto ret = system->tick(sim::api::Scheduler::Mode::Jump);
    noMMI = ret.second.error == sim::api::tick::Error::NoMMInput;
    fail |= ret.second.error != sim::api::tick::Error::Success;
    if (fail)
      break;
  }
  if (noMMI) {
    std::cout << "Program request data from charIn, but no data is present. "
                 "Terminating.\n";
  }
  if (system->currentTick() >= _maxSteps) {
    std::cout << "Exceeded max number of steps. Possible infinite loop\n";
    // Write to console that an infinite loop was detected.
  }
  if (auto charOut = system->output("charOut"); !_charOut.empty() && charOut) {
    auto charOutEndpoint = charOut->endpoint();
    charOutEndpoint->set_to_head();
    auto writeOut = [&](QTextStream &outF) {
      for (auto next = charOutEndpoint->next_value(); next.has_value();
           next = charOutEndpoint->next_value()) {
        outF << char(*next);
      }
    };

    if (_charOut == "-") {
        QTextStream out(stdout, QIODevice::WriteOnly | QIODevice::Truncate |
                                  QIODevice::Text);
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
      memDump.write(reinterpret_cast<const char *>(dump.constData()),
                    dump.size());
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

void RunTask::setSkipLoad(bool skip) { _skipLoad = skip; }

void RunTask::setSkipDispatch(bool skip) { _skipDispatch = skip; }

void RunTask::addRegisterOverride(std::string name, quint16 value) {
  _regOverrides[name] = value;
}
