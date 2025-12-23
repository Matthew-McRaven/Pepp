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
#include "sim3/subsystems/ram/dense.hpp"
#include "targets/isa3/helpers.hpp"
#include "targets/pep9/isa3/cpu.hpp"
#include "utils/bits/swap.hpp"

static inline sim::api2::memory::Operation rw = {
    .type = sim::api2::memory::Operation::Type::Standard,
    .kind = sim::api2::memory::Operation::Kind::data,
};

static inline std::pair<QSharedPointer<sim::memory::Dense<quint16>>, QSharedPointer<targets::pep9::isa::CPU>> make() {
  int i = 3;

  static const auto desc_mem = sim::api2::device::Descriptor{
      .id = 1,
      .baseName = "ram",
      .fullName = "/ram",
  };

  static const auto desc_cpu = sim::api2::device::Descriptor{
      .id = 2,
      .baseName = "cpu",
      .fullName = "/cpu",
  };

  static const auto span = sim::api2::memory::AddressSpan<quint16>(0, 0xFFFF);
  sim::api2::device::IDGenerator gen = [&i]() { return i++; };
  auto storage = QSharedPointer<sim::memory::Dense<quint16>>::create(desc_mem, span);
  auto cpu = QSharedPointer<targets::pep9::isa::CPU>::create(desc_cpu, gen);
  cpu->setTarget(storage.data(), nullptr);
  return std::pair{storage, cpu};
};

static inline auto reg(QSharedPointer<targets::pep9::isa::CPU> cpu, isa::Pep9::Register reg) -> quint16 {
  quint16 tmp = 0;
  targets::isa::readRegister<isa::Pep9>(cpu->regs(), reg, tmp, rw);
  return tmp;
};

static inline quint16 csr(QSharedPointer<targets::pep9::isa::CPU> cpu, isa::Pep9::CSR csr) {
  bool tmp = 0;
  targets::isa::readCSR<isa::Pep9>(cpu->csrs(), csr, tmp, rw);
  return tmp;
};
