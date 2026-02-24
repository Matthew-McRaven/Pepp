/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
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
#include "features.hpp"
#include "project/architectures.hpp"
#include "sim3/api/traced/memory_target.hpp"
#include "sim3/api/traced_system.hpp"
#include "sim3/cores/pep/traced_pep9_mc2.hpp"
#include "sim3/subsystems/bus/simple.hpp"

namespace sim {
namespace memory {
template <typename Address> class Dense;
template <typename Address> class SimpleBus;
} // namespace memory
} // namespace sim

namespace targets::ma {
class System : public sim::api2::System<quint16> {
public:
  System(pepp::Architecture arch, pepp::Features feats);
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
  targets::pep9::mc2::BaseCPU *cpu();
  sim::memory::SimpleBus<quint16> *bus();

private:
  sim::api2::device::ID _nextID = 0;
  sim::api2::device::IDGenerator _nextIDGenerator = [this]() { return _nextID++; };
  sim::api2::tick::Type _tick = 0;

  pepp::Architecture _arch = pepp::Architecture::NO_ARCH;
  pepp::Features _feats = pepp::Features::None;
  QSharedPointer<targets::pep9::mc2::BaseCPU> _cpu = nullptr;
  QSharedPointer<sim::memory::Dense<quint16>> _rawMemory = nullptr;
  QSharedPointer<sim::memory::SimpleBus<quint16>> _bus = nullptr;
  QSharedPointer<sim::api2::Paths> _paths = nullptr;
  QMap<sim::api2::device::ID, sim::api2::device::Descriptor> _devices = {};
};
} // namespace targets::ma
