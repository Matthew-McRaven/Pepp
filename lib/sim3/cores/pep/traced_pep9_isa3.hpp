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

#pragma once
#include "../../../../core/core/arch/pep/isa/pep9.hpp"
#include "sim/debug/debugger.hpp"
#include "sim3/subsystems/ram/dense.hpp"

namespace sim::memory {
template <typename Address> class Output;
} // namespace sim::memory

namespace targets::pep9::isa {
class CPU : public sim::api2::tick::Recipient,
            public sim::api2::trace::Source,
            public sim::api2::trace::Sink,
            public sim::api2::memory::Initiator<quint16> {

public:
  CPU(sim::api2::device::Descriptor device, sim::api2::device::IDGenerator gen);
  ~CPU() = default;
  CPU(CPU &&other) noexcept = default;
  CPU &operator=(CPU &&other) = default;
  CPU(const CPU &) = delete;
  CPU &operator=(const CPU &) = delete;

  sim::api2::memory::Target<quint8> *regs();
  sim::api2::memory::Target<quint8> *csrs();
  sim::api2::device::Descriptor device() const;
  enum class Status {
    Ok = 0,
    IllegalOpcode = 1,
  };

  Status status() const;
  // Helper to implement STOP opcode.
  void setPwrOff(sim::memory::Output<quint16> *pwrOff);
  // Helper to convert OS to an operand value.
  // It will use Application access, and will not trigger MMIO.
  std::optional<quint16> currentOperand();
  quint16 startingPC() const;
  // Set the starting PC to the current PC. Needed to get 1st step correct.
  void updateStartingPC();
  quint16 depth() const;

  // Target interface
  const sim::api2::tick::Source *getSource() override;
  void setSource(sim::api2::tick::Source *) override;
  sim::api2::tick::Result clock(sim::api2::tick::Type currentTick) override;

  // Sink interfae
  bool analyze(sim::api2::trace::PacketIterator iter, sim::api2::trace::Direction) override;

  // Source interface
  void trace(bool enabled) override;
  void setBuffer(sim::api2::trace::Buffer *tb) override;
  const sim::api2::trace::Buffer *buffer() const override { return _tb; }

  // Initiator interface
  void setTarget(sim::api2::memory::Target<quint16> *target, void *port) override;

  void setDebugger(pepp::debug::Debugger *debugger);
  void clearDebugger();

private:
  // Increment depth and emit a trace packet.
  void incrDepth();
  void decrDepth();
  // TODO: This probably needs to be cleared between simulations
  quint16 _depth = 0, _startingPC = 0;
  Status _status = Status::Ok;
  sim::api2::device::Descriptor _device;
  sim::memory::Dense<quint8> _regs, _csrs;
  sim::api2::memory::Target<quint16> *_memory;
  sim::memory::Output<quint16> *_pwrOff = nullptr;

  sim::api2::tick::Source *_clock = nullptr;
  sim::api2::trace::Buffer *_tb = nullptr;
  pepp::debug::Debugger *_dbg = nullptr;

  quint16 readReg(::isa::Pep9::Register reg);
  void writeReg(::isa::Pep9::Register reg, quint16 val);
  bool readCSR(::isa::Pep9::CSR csr);
  void writeCSR(::isa::Pep9::CSR csr, bool val);
  quint8 readPackedCSR();
  void writePackedCSR(quint8 val);

  sim::api2::tick::Result unaryDispatch(quint8 is, quint16 pc);
  sim::api2::tick::Result nonunaryDispatch(quint8 is, quint16 os, quint16 pc);
  void decodeStoreOperand(quint8 is, quint16 os, quint16 &decoded, bool traced = true);
  void decodeLoadOperand(quint8 is, quint16 os, quint16 &decoded, bool traced = true);
};
} // namespace targets::pep9::isa
