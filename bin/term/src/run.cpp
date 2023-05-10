#include "./run.hpp"
#include "./shared.hpp"
#include "bits/strings.hpp"
#include "builtins/figure.hpp"
#include "obj/mmio.hpp"
#include "sim/device/broadcast/mmi.hpp"
#include "sim/device/broadcast/mmo.hpp"
#include "sim/device/simple_bus.hpp"
#include "targets/pep10/isa3/cpu.hpp"
#include "targets/pep10/isa3/helpers.hpp"
#include "targets/pep10/isa3/system.hpp"
#include <sstream>

auto gs = sim::api::memory::Operation{
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
  if (!_osIn.has_value()) {
    auto os = book->findFigure("os", "full");
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
  auto bytes = bits::asciiHexToByte(objText.data(), objText.size());
  if (!bytes)
    return false;
  _elf = helper.elf(*bytes);
  _elf->save("tmp.elf");
  _elf->load("tmp.elf");
  return true;
}

void RunTask::run() {
  if (!loadToElf())
    return emit finished(1);
  bool loadIm = false | !(obj::getBootFlagsAddress(*_elf).has_value());
  auto system = targets::pep10::isa::systemFromElf(*_elf, loadIm);
  system->init();
  system->setBootFlags(!loadIm, true);
  /*targets::pep10::isa::writeRegister(system->cpu()->regs(),
                                     isa::Pep10::Register::PC, 0, gs);*/
  auto pwrOff = system->output("pwrOff");
  auto endpoint = pwrOff->endpoint();
  bool fail = false;
  if (auto charIn = system->input("charIn"); !_charIn.empty() && charIn) {
    auto charInEndpoint = charIn->endpoint();
    QString buffer;
    if (_charIn == "-") {
      QFile stdF;
      stdF.open(0, QIODevice::ReadOnly | QIODevice::Text);
      buffer = stdF.readAll();
    } else {
      QFile f(QString::fromStdString(_charIn));
      f.open(QIODevice::ReadOnly | QIODevice::Text);
      buffer = f.readAll();
    }
    auto asStd = buffer.toStdString();
    for (int it = 0; it < asStd.size(); it++)
      charInEndpoint->append_value(asStd[it]);
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
    std::cout << "Program request data from charIn, but not data is present. "
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
      outF << "\n";
    };

    if (_charOut == "-") {
      QTextStream out(stdout, QIODevice::WriteOnly | QIODevice::Truncate |
                                  QIODevice::Text);
      writeOut(out);
    } else {
      QFile f(QString::fromStdString(_charOut));
      f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
      QTextStream out(&f);
      writeOut(out);
    }
  }

  if (!_memDump.empty()) {
    QVector<quint8> dump(0x1'00'00);
    system->bus()->dump(dump.data(), dump.size());
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

void RunTask::setOsIn(std::string fname) { _osIn = fname; }
