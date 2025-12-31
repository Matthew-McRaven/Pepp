#pragma once
/*
 * Copyright (c) 2023-2024 J. Stanley Warford, Matthew McRaven
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
#include "sim3/subsystems/ram/dense.hpp"
#include "sim3/trace/modified.hpp"

namespace sim::memory {
class IDEController : public sim::api2::trace::Source,
                      public sim::api2::trace::Sink,
                      public sim::api2::memory::Target<quint16>,
                      public sim::api2::memory::Initiator<quint16> {
public:
  using AddressSpan = typename api2::memory::AddressSpan<quint16>;
  IDEController(api2::device::Descriptor device, quint16 base, sim::api2::device::IDGenerator gen);
  ~IDEController() = default;
  IDEController(IDEController &&other) noexcept = default;
  IDEController &operator=(IDEController &&other) = default;
  // Disable copy construction and assignment, since it would be incorrect for
  // multiple objects to share a device descriptor.
  IDEController(const IDEController &) = delete;
  IDEController &operator=(const IDEController &) = delete;
#pragma pack(push, 1)
  struct IDERegs {
    quint8 ideCMD;
    quint8 offLBA;
    quint16 LBA;
    quint16 addrDMA;
    quint16 lenDMA;
  };
#pragma pack(pop)
  static const quint32 sectorSize = 256;
  enum class RegisterOffsets : quint8 {
    ideCMD = 0,
    offLBA = 1,
    LBA = 2,
    addrDMA = 4,
    lenDMA = 6,
  };
  enum class Commands {
    READ_DMA = 0xC9,  // IDE command: READ DMA w/o retry
    WRITE_DMA = 0xCB, // IDE command: IDE WRTIE DMA w/o retry
    ERASE = 0x50,     // IDE command: FORMAT TRACK
  };

  IDERegs regs() const;
  void setRegs(IDERegs regs, bool triggerExec = false);
  void execute(Commands command);
  const sim::memory::Dense<quint32> *disk() const;
  sim::memory::Dense<quint32> *disk();

  // Initiator interface
  void setTarget(sim::api2::memory::Target<quint16> *target, void *port) override;

  // Sink interface
  bool analyze(api2::trace::PacketIterator iter, api2::trace::Direction direction) override;

  // Target interface (for registers!)
  sim::api2::device::ID deviceID() const override { return _device.id; }
  sim::api2::device::Descriptor device() const override { return _device; }
  AddressSpan span() const override;
  api2::memory::Result read(quint16 address, bits::span<quint8> dest, api2::memory::Operation op) const override;
  api2::memory::Result write(quint16 address, bits::span<const quint8> src, api2::memory::Operation op) override;
  void clear(quint8 fill) override;
  void dump(bits::span<quint8> dest) const override;

  // Source interface
  void setBuffer(api2::trace::Buffer *tb) override;
  const api2::trace::Buffer *buffer() const override;
  void trace(bool enabled) override;

private:
  api2::device::Descriptor _device;
  sim::api2::memory::Target<quint16> *_target = nullptr;
  sim::memory::Dense<quint16> _regs;
  sim::memory::Dense<quint32> _disk;
  std::array<quint8, sectorSize> _buffer;

  // Helper to enable RAII for _inExec.
  class ExecGuard {
  public:
    ExecGuard(bool *inExec) : _inExec(inExec) {
      if (_inExec) (*_inExec) = true;
    }
    ~ExecGuard() {
      if (_inExec) (*_inExec) = false;
    }
    ExecGuard(const ExecGuard &) = delete;
    ExecGuard &operator=(const ExecGuard &) = delete;
    ExecGuard(ExecGuard &&) = default;
    ExecGuard &operator=(ExecGuard &&) = default;

  private:
    bool *_inExec = nullptr;
  };
  // Ensure that an exec_command cannot copy data into IDE's control registers, causing an endless loop.
  bool _inExec = false;
  void exec_command();
};

} // namespace sim::memory
