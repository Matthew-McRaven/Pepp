/*
 * Copyright (c) 2024 J. Stanley Warford, Matthew McRaven
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
#include <qtypes.h>
#include "bits/operations/swap.hpp"
#include "sim/device/dense.hpp"
#include "targets/pep10/isa3/cpu.hpp"
#include "targets/pep10/isa3/helpers.hpp"

auto desc_mem = sim::api2::device::Descriptor{
    .id = 1,
    .baseName = "ram",
    .fullName = "/ram",
};

auto desc_cpu = sim::api2::device::Descriptor{
    .id = 2,
    .baseName = "cpu",
    .fullName = "/cpu",
};

auto span = sim::api2::memory::AddressSpan<quint16>{
    .minOffset = 0,
    .maxOffset = 0xFFFF,
};

sim::api2::memory::Operation rw = {
    .type = sim::api2::memory::Operation::Type::Standard,
    .kind = sim::api2::memory::Operation::Kind::data,
};

inline auto make = []() {
  int i = 3;
  sim::api2::device::IDGenerator gen = [&i]() { return i++; };
  auto storage = QSharedPointer<sim::memory::Dense<quint16>>::create(desc_mem, span);
  auto cpu = QSharedPointer<targets::pep10::isa::CPU>::create(desc_cpu, gen);
  cpu->setTarget(storage.data(), nullptr);
  return std::pair{storage, cpu};
};

inline auto reg(QSharedPointer<targets::pep10::isa::CPU> cpu, isa::Pep10::Register reg) -> quint16 {
  quint16 tmp = 0;
  targets::pep10::isa::readRegister(cpu->regs(), reg, tmp, rw);
  return tmp;
};

inline quint16 csr(QSharedPointer<targets::pep10::isa::CPU> cpu, isa::Pep10::CSR csr) {
  bool tmp = 0;
  targets::pep10::isa::readCSR(cpu->csrs(), csr, tmp, rw);
  return tmp;
};
