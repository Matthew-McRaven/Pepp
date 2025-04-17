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

#include "throughput.hpp"
#include "sim/device/dense.hpp"
#include "targets/pep10/isa3/cpu.hpp"
#include <chrono>
#include <iostream>
const auto desc_mem = sim::api2::device::Descriptor{
    .id = 1,
    .baseName = "ram",
    .fullName = "/ram",
};

const auto desc_cpu = sim::api2::device::Descriptor{
    .id = 2,
    .baseName = "cpu",
    .fullName = "/cpu",
};

const auto span = sim::api2::memory::AddressSpan<quint16>(0, 0xFFFF);

auto make = []() {
  int i = 3;
  sim::api2::device::IDGenerator gen = [&i]() { return i++; };
  auto storage = QSharedPointer<sim::memory::Dense<quint16>>::create(desc_mem, span);
  auto cpu = QSharedPointer<targets::pep10::isa::CPU>::create(desc_cpu, gen);
  cpu->setTarget(storage.data(), nullptr);
  return std::pair{storage, cpu};
};

sim::api2::memory::Operation rw = {
    .type = sim::api2::memory::Operation::Type::Standard,
    .kind = sim::api2::memory::Operation::Kind::data,
};

ThroughputTask::ThroughputTask(QObject *parent) : Task(parent) {}

void ThroughputTask::run() {
  using namespace Qt::StringLiterals;
  // Add some spurious breakpoints which will not be hit
  auto debugger = std::make_shared<pepp::debug::Debugger>(nullptr);
  for (int it = 0; it < 128; it++) debugger->bps->addBP(2048 + it);
  auto [mem, cpu] = make();
  cpu->setDebugger(&*debugger);
  cpu->regs()->clear(0);
  cpu->csrs()->clear(0);
  // Infinite looping branch to 0.
  auto program = std::array<quint8, 3>{static_cast<quint8>(isa::Pep10::Mnemonic::BR), 0x00, 0x00};
  mem->write(0, {program.data(), program.size()}, rw);
  auto start = std::chrono::high_resolution_clock::now();
  auto maxInstr = 1'000'000;
  for (int it = 0; it < maxInstr; it++) cpu->clock(it);
  auto end = std::chrono::high_resolution_clock::now();
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  auto dt = 1.0 / (ms.count() / 1000.0);
  std::cout << u"Duration was: %1\n"_s.arg(ms.count()).toStdString();
  std::cout << u"Throughput was: %1\n"_s.arg(dt * maxInstr).toStdString();

  emit finished(0);
}
