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
#include "core/arch/pep/uarch/pep.hpp"
#include "core/langs/ucode/pep_ir.hpp" // TODO: Stop including the parser in the simulator!!
#include "sim/debug/debugger.hpp"
#include "sim3/api/traced/memory_target.hpp"
#include "sim3/api/traced/trace_endpoint.hpp"
#include "sim3/subsystems/ram/dense.hpp"

namespace sim::memory {
template <typename Address> class Output;
} // namespace sim::memory

namespace targets::pep9::mc2 {

// Used to share logic & data between 1-byte and 2-byte CPU implementations.
// Data is shared across class heirarchy using protected, which is going to make analyzers angry.
class BaseCPU : public sim::api2::tick::Recipient,
                public sim::api2::trace::Source,
                public sim::api2::trace::Sink,
                public sim::api2::memory::Initiator<quint16> {
public:
  using CSRs = pepp::tc::arch::Pep9Registers::CSRs;
  BaseCPU(sim::api2::device::Descriptor device, sim::api2::device::IDGenerator gen, quint8 hiddenRegCount);
  virtual ~BaseCPU() = 0;
  BaseCPU(BaseCPU &&other) noexcept = default;
  BaseCPU &operator=(BaseCPU &&other) = default;
  BaseCPU(const BaseCPU &) = delete;
  BaseCPU &operator=(const BaseCPU &) = delete;

  sim::api2::memory::Target<quint8> *bankRegs();
  sim::api2::memory::Target<quint8> *hiddenRegs();
  sim::api2::memory::Target<quint8> *csrs();
  sim::api2::device::Descriptor device() const;
  enum class Status { Ok = 0, Halted, ChangedAddress, ChangedData, MemoryTooSoon };
  Status status() const;
  void setConstantRegisters();
  void resetMicroPC();

  // Target interface
  const sim::api2::tick::Source *getSource() override;
  void setSource(sim::api2::tick::Source *) override;

  // Source interface
  void trace(bool enabled) override;
  void setBuffer(sim::api2::trace::Buffer *tb) override;
  const sim::api2::trace::Buffer *buffer() const override { return _tb; }

  // Initiator interface
  void setTarget(sim::api2::memory::Target<quint16> *target, void *port) override;

  void setDebugger(pepp::debug::Debugger *debugger);
  void clearDebugger();

protected:
  Status _status = Status::Ok;
  quint16 _microPC = 0;
  sim::api2::device::Descriptor _device;
  sim::memory::Dense<quint8> _bankRegs, _hiddenRegs, _csrs;
  sim::api2::memory::Target<quint16> *_memory;

  sim::api2::tick::Source *_clock = nullptr;
  sim::api2::trace::Buffer *_tb = nullptr;
  pepp::debug::Debugger *_dbg = nullptr;

  quint8 readReg(quint8 reg);
  void writeReg(quint8 reg, quint8 val);
  bool readCSR(CSRs reg);
  void writeCSR(CSRs reg, bool val);

  struct MemoryTransaction {
    quint8 onCycle = 0;
  } memStatus;
};

class CPUByteBus : public BaseCPU {
public:
  using HiddenRegisters = pepp::tc::arch::Pep9ByteBus::HiddenRegisters;
  CPUByteBus(sim::api2::device::Descriptor device, sim::api2::device::IDGenerator gen);
  ~CPUByteBus() = default;
  CPUByteBus(CPUByteBus &&other) noexcept = default;
  CPUByteBus &operator=(CPUByteBus &&other) = default;
  CPUByteBus(const CPUByteBus &) = delete;
  CPUByteBus &operator=(const CPUByteBus &) = delete;

  void setMicrocode(std::vector<pepp::tc::arch::Pep9ByteBus::Code> &&code);
  const std::span<const pepp::tc::arch::Pep9ByteBus::Code> microcode();
  void applyPreconditions(const std::vector<pepp::tc::ir::Test<pepp::tc::arch::Pep9Registers>> &tests);
  std::vector<bool> testPostconditions(const std::vector<pepp::tc::ir::Test<pepp::tc::arch::Pep9Registers>> &tests);

  // Target interface
  sim::api2::tick::Result clock(sim::api2::tick::Type currentTick) override;

  // Sink interfae
  bool analyze(sim::api2::trace::PacketIterator iter, sim::api2::trace::Direction) override;

private:
  std::vector<pepp::tc::arch::Pep9ByteBus::Code> _microcode;
  quint8 readHidden(HiddenRegisters reg);
  void writeHidden(HiddenRegisters reg, quint8 val);
};
class CPUWordBus : public BaseCPU {
public:
  using HiddenRegisters = pepp::tc::arch::Pep9WordBus::HiddenRegisters;
  CPUWordBus(sim::api2::device::Descriptor device, sim::api2::device::IDGenerator gen);
  ~CPUWordBus() = default;
  CPUWordBus(CPUWordBus &&other) noexcept = default;
  CPUWordBus &operator=(CPUWordBus &&other) = default;
  CPUWordBus(const CPUWordBus &) = delete;
  CPUWordBus &operator=(const CPUWordBus &) = delete;

  void setMicrocode(std::vector<pepp::tc::arch::Pep9WordBus::Code> &&code);
  const std::span<const pepp::tc::arch::Pep9WordBus::Code> microcode();
  void applyPreconditions(const std::vector<pepp::tc::ir::Test<pepp::tc::arch::Pep9Registers>> &tests);
  std::vector<bool> testPostconditions(const std::vector<pepp::tc::ir::Test<pepp::tc::arch::Pep9Registers>> &tests);

  // Target interface
  sim::api2::tick::Result clock(sim::api2::tick::Type currentTick) override;

  // Sink interfae
  bool analyze(sim::api2::trace::PacketIterator iter, sim::api2::trace::Direction) override;

private:
  std::vector<pepp::tc::arch::Pep9WordBus::Code> _microcode;
  quint8 readHidden(HiddenRegisters reg);
  void writeHidden(HiddenRegisters reg, quint8 val);
};
} // namespace targets::pep9::mc2
