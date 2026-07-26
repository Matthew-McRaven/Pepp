/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
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
#include "core/arch/pep/isa/pep10.hpp"
#include "core/arch/pep/isa/pep9.hpp"
#include "core/sim/cores/cpu/pep_isa.hpp"
#include "core/sim/cores/cpu/pep_isa_instructions.hpp"
#include "core/sim/memory/ram/dense.hpp"
#include "core/sim/system.hpp"

namespace {

// First return value is the system (don't drop it!) and the second is a ptr to the CPU.
inline auto make_cpu(PepISA3CPU::ISA isa = PepISA3CPU::ISA::Pep10) {
  using namespace bits;
  PepISA3CPU::Configuration cpu_cfg{Device::Configuration{
                                        .basename = "cpu",
                                        .compatible = PepISA3CPU::compatible,
                                    },
                                    isa, "/memory"};
  System::Configuration root_cfg{{.basename = "/", .compatible = System::compatible}};
  Dense::Configuration mem_cfg{
      Device::Configuration{
          .basename = "memory",
          .compatible = Dense::compatible,
      },
      0x00,
      AddressSpan(0x0000, 0xffff),
  };
  auto system = std::make_unique<System>(root_cfg);
  auto *mem = system->make_device<Dense>(mem_cfg);
  auto *cpu = system->make_device<PepISA3CPU>(cpu_cfg, system.get());
  system->initialize();
  return std::make_tuple(std::move(system), mem, cpu);
}

static const Operation rw{.type = Operation::Type::Standard, .kind = Operation::Kind::data};

template <typename Register, typename CSR, typename Mnemonic> void inner_call(PepISA3CPU::ISA isa, Mnemonic op) {
  auto [sys, mem, cpu] = make_cpu(isa);
  // Object code for instruction under test.
  auto program = std::array<u8, 3>{(u8)op, 0xDE, 0xAD};
  REQUIRE_NOTHROW(mem->write(0, {program.data(), program.size()}, rw));
  const u16 init_sp = 0xFEED;
  const u16 end_pc = 0xDEAD;

  cpu->registers()->clear(0);
  cpu->csrs()->clear(0);
  cpu->write_register(Register::SP, init_sp);
  cpu->write_packed_csr(pack_csr(true, false, true, false)); // NZVC = 1010

  REQUIRE_NOTHROW(cpu->clock_tick(PulseSchedule::PulseIndex{0}, 0));

  auto dbg = sys->register_scan();

  CHECK(dbg->read<u16>(*dbg->find("PC")) == end_pc);
  CHECK(dbg->read<u16>(*dbg->find("SP")) == init_sp - 2);
  const auto sp = dbg->read<u16>(*dbg->find("SP"));
  const auto mem_sp = ((Target *)mem)->read<u16, bits::host_is_le>(sp, rw).second;
  CHECK(mem_sp == 0x0003); // The return address is 0x0003, since that was the next PC prior to call.
  CHECK(dbg->read<u8>(*dbg->find("IS")) == (u8)op);
  CHECK(dbg->read<u16>(*dbg->find("OS")) == 0xDEAD);
  CHECK(dbg->read<u16>(*dbg->find("X")) == 0);
  CHECK(dbg->read<u16>(*dbg->find("A")) == 0);
  CHECK(dbg->read<u8>(*dbg->find("N")) == true);
  CHECK(dbg->read<u8>(*dbg->find("Z")) == false);
  CHECK(dbg->read<u8>(*dbg->find("V")) == true);
  CHECK(dbg->read<u8>(*dbg->find("C")) == false);
}

} // namespace

TEST_CASE("Find register by name in HW debugger", "[scope:core][scope:core.sim][kind:instr][arch:pep10]") {
  using Register = isa::Pep10::Register;
  using CSR = isa::Pep10::CSR;
  using MN = isa::Pep10::Mnemonic;
  inner_call<Register, CSR, MN>(PepISA3CPU::ISA::Pep10, MN::CALL);
}
