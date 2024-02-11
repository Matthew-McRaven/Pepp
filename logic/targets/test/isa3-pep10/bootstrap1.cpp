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

#include <catch.hpp>
#include "bits/operations/swap.hpp"
#include "sim/device/dense.hpp"
#include "targets/pep10/isa3/cpu.hpp"
#include "targets/pep10/isa3/helpers.hpp"
sim::api2::memory::Operation rw = {
    .type = sim::api2::memory::Operation::Type::Standard,
    .kind = sim::api2::memory::Operation::Kind::data,
};

TEST_CASE("Targets_ISA3Pep10_Bootstrap1") {
  sim::api2::device::ID id = 0;
  auto nextID = [&id]() { return id++; };
  auto desc_mem =
      sim::api2::device::Descriptor{.id = nextID(), .compatible = nullptr, .baseName = "dev", .fullName = "/dev"};
  auto span = sim::api2::memory::AddressSpan<quint16>{.minOffset = 0, .maxOffset = 0xFFFF};
  sim::memory::Dense<quint16> mem(desc_mem, span, (int)isa::Pep10::Mnemonic::NOP);
  auto desc_cpu =
      sim::api2::device::Descriptor{.id = nextID(), .compatible = nullptr, .baseName = "cpu", .fullName = "/cpu"};
  targets::pep10::isa::CPU cpu(desc_cpu, nextID);
  cpu.setTarget(&mem, nullptr);
  auto regs = cpu.regs();
  quint16 tmp = 0;

  using Register = isa::Pep10::Register;
  // Check that PC is incremented when executing NOP.
  REQUIRE_NOTHROW(targets::pep10::isa::readRegister(regs, Register::PC, tmp, rw));
  CHECK(tmp == 0);

  auto tick = cpu.clock(0);

  REQUIRE_NOTHROW(targets::pep10::isa::readRegister(regs, Register::PC, tmp, rw));
  CHECK(tmp == 1);

  // Check that A can be modified.
  quint8 v = (quint8)isa::Pep10::Mnemonic::NOTA;
  REQUIRE_NOTHROW(mem.write(0x01, {&v, 1}, rw));
  REQUIRE_NOTHROW(targets::pep10::isa::readRegister(regs, Register::A, tmp, rw));
  CHECK(tmp == 0);

  tick = cpu.clock(1);

  REQUIRE_NOTHROW(targets::pep10::isa::readRegister(regs, Register::A, tmp, rw));
  CHECK(tmp == 0xFFFF);
}
