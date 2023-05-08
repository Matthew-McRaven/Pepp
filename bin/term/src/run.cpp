#include "./run.hpp"
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

RunTask::RunTask(const ELFIO::elfio &elf, QObject *parent)
    : Task(parent), _elf(elf) {}

void RunTask::run() {
  bool loadIm = false | !(obj::getBootFlagsAddress(_elf).has_value());
  auto system = targets::pep10::isa::systemFromElf(_elf, loadIm);
  system->init();
  system->setBootFlags(!loadIm, true);
  /*targets::pep10::isa::writeRegister(system->cpu()->regs(),
                                     isa::Pep10::Register::PC, 0, gs);*/
  auto pwrOff = system->output("pwrOff");
  auto endpoint = pwrOff->endpoint();
  bool fail = false;
  if (auto charIn = system->input("charIn"); !_charIn.empty() && charIn) {
    auto charInEndpoint = charIn->endpoint();
    std::stringstream buffer;
    if (_charIn == "-") {
      QFile stdF;
      stdF.open(0, QIODevice::OpenModeFlag::ReadOnly);
      auto x = stdF.readAll();
      buffer << x.toStdString();
      stdF.close();
    } else {
      std::ifstream t(_charIn);
      buffer << t.rdbuf();
    }
    auto str = buffer.str();
    std::cout << "[ CIN]<" << str << std::endl;
    for (int it = 0; it < str.size(); it++)
      charInEndpoint->append_value(str[it]);
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
    quint16 tmp = 0;
    targets::pep10::isa::readRegister(system->cpu()->regs(),
                                      isa::Pep10::Register::IS, tmp, gs);
    auto instr = isa::Pep10::opcodeLUT[tmp];
    std::cout << u"%1"_qs
                     .arg(QMetaEnum::fromType<isa::detail::pep10::Mnemonic>()
                              .valueToKey((int)instr.instr.mnemon),
                          7)
                     .toStdString();
    if (!instr.instr.unary) {
      targets::pep10::isa::readRegister(system->cpu()->regs(),
                                        isa::Pep10::Register::OS, tmp, gs);
      std::cout
          << u" %1, "_qs.arg(QString::number(tmp, 16), 4, '0').toStdString();
      std::cout
          << u"%1"_qs
                 .arg(QMetaEnum::fromType<isa::detail::pep10::AddressingMode>()
                          .valueToKey((int)instr.mode),
                      3)
                 .toStdString()
          << ";";
    } else {
      std::cout << "          ;";
    }
    printReg(isa::Pep10::Register::PC);
    printReg(isa::Pep10::Register::A);
    printReg(isa::Pep10::Register::X);
    printReg(isa::Pep10::Register::SP);
    printReg(isa::Pep10::Register::TR);
    std::cout << std::endl;
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
    auto writeOut = [&](std::ostream &outF) {
      for (auto next = charOutEndpoint->next_value(); next.has_value();
           next = charOutEndpoint->next_value()) {
        outF << *next;
      }
    };
    if (!_memDump.empty()) {
      QVector<quint8> dump(0x1'00'00);
      system->bus()->dump(dump.data(), dump.size());
      QFile memDump(QString::fromStdString(_memDump));
      if (memDump.open(QFile::WriteOnly)) {
        memDump.write(reinterpret_cast<const char *>(dump.constData()),
                      dump.size());
        memDump.close();
      }
    }
    if (_charOut == "-")
      writeOut(std::cout);
    else {
      std::ofstream out(_charOut);
      writeOut(out);
    }
  }
  emit finished(0);
}

void RunTask::setCharOut(std::string fname) { this->_charOut = fname; }

void RunTask::setCharIn(std::string fname) { this->_charIn = fname; }

void RunTask::setMemDump(std::string fname) { _memDump = fname; }

void RunTask::setMaxSteps(quint64 maxSteps) { this->_maxSteps = maxSteps; }
