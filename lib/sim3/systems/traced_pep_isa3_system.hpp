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
#include <elfio/elfio.hpp>
#include "project/architectures.hpp"
#include "project/levels.hpp"
#include "sim3/api/traced/memory_target.hpp"
#include "sim3/api/traced_pep10_isa3_system.hpp"
#include "toolchain/link/memmap.hpp"

namespace obj {
struct MemoryRegion;
struct AddressedIO;
} // namespace obj
namespace sim {
namespace memory {
class IDEController;
template <typename Address> class Dense;
template <typename Address> class SimpleBus;
template <typename Address> class Input;
template <typename Address> class Output;
template <typename Address> class ReadOnly;
} // namespace memory
} // namespace sim

namespace targets::isa {
class System : public sim::api2::System<quint16> {
public:
  System(pepp::Architecture arch, QList<obj::MemoryRegion> regions, QList<obj::AddressedIO> mmios);
  // System interface
  std::pair<sim::api2::tick::Type, sim::api2::tick::Result> tick(sim::api2::Scheduler::Mode mode) override;
  sim::api2::tick::Type currentTick() const override;
  sim::api2::device::ID nextID() override;
  sim::api2::device::IDGenerator nextIDGenerator() override;
  void addDevice(sim::api2::device::Descriptor) override;
  sim::api2::device::Descriptor *descriptor(sim::api2::device::ID) override;
  void setBuffer(sim::api2::trace::Buffer *buffer) override;
  QSharedPointer<const sim::api2::Paths> pathManager() const override;

  // Set default register values.
  void init();

  pepp::Architecture architecture() const;
  sim::api2::tick::Recipient *cpu();

  sim::memory::SimpleBus<quint16> *bus();
  QStringList inputs() const;
  sim::memory::Input<quint16> *input(QString name);
  QStringList outputs() const;
  sim::memory::Output<quint16> *output(QString name);
  QStringList ideControllers() const;
  sim::memory::IDEController *ideController(QString name);

  void doReloadEntries();
  void reconfigure(const ELFIO::elfio &elf);

private:
  void reconfigure(pepp::Architecture arch, QList<obj::MemoryRegion> regions, QList<obj::AddressedIO> mmios);
  sim::api2::device::ID _nextID = 0;
  sim::api2::device::IDGenerator _nextIDGenerator = [this]() { return _nextID++; };
  sim::api2::tick::Type _tick = 0;
  struct ReloadHelper {
    QSharedPointer<sim::api2::memory::Target<quint16>> target;
    quint16 base;
    std::vector<quint8> data;
  };
  void appendReloadEntries(QSharedPointer<sim::api2::memory::Target<quint16>> mem, const obj::MemoryRegion &reg,
                           quint16 baseOffset = 0);
  QList<ReloadHelper> _regions;

  const pepp::Architecture _arch = pepp::Architecture::NO_ARCH;
  QSharedPointer<sim::api2::tick::Recipient> _cpu = nullptr;
  QSharedPointer<sim::memory::SimpleBus<quint16>> _bus = nullptr;
  QSharedPointer<sim::api2::Paths> _paths = nullptr;
  QVector<QSharedPointer<sim::memory::Dense<quint16>>> _rawMemory = {};
  QVector<QSharedPointer<sim::memory::ReadOnly<quint16>>> _readonly = {};

  QMap<QString, QSharedPointer<sim::memory::Input<quint16>>> _mmi = {};
  QMap<QString, QSharedPointer<sim::memory::Output<quint16>>> _mmo = {};
  QMap<QString, QSharedPointer<sim::memory::IDEController>> _ide = {};
  QMap<sim::api2::device::ID, sim::api2::device::Descriptor> _devices = {};
};

bool loadRegion(sim::api2::memory::Target<quint16> &mem, const obj::MemoryRegion &reg, quint16 baseOffset = 0);
// Do not buffer any MMIO values.
bool loadElfSegments(sim::api2::memory::Target<quint16> &mem, const ELFIO::elfio &elf);

// loadUserImmediate bypasses loading user program to DDR.
QSharedPointer<System> systemFromElf(const ELFIO::elfio &elf, bool loadUserImmediate);
} // namespace targets::isa
