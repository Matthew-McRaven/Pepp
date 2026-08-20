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

static const Operation rw{Operation::Type::Standard, Operation::Kind::data};

template <typename Register, typename CSR, typename Mnemonic> void inner_call(PepISA3CPU::ISA isa, Mnemonic op) {
  auto [sys, mem, cpu] = make_cpu(isa);
  // Object code for instruction under test.
  auto program = std::array<u8, 3>{(u8)op, 0xDE, 0xAD};
  auto dbg = sys->register_scan();
  auto mid = mem->id();
  auto rd_bytes = dbg->find("rd_bytes", mid);
  auto wr_bytes = dbg->find("wr_bytes", mid);
  REQUIRE(rd_bytes.has_value());
  REQUIRE(wr_bytes.has_value());
  auto resolve_wr = dbg->resolve(*wr_bytes);
  CHECK(resolve_wr.first != nullptr);
  CHECK((i16)resolve_wr.first->target.value == (i16)mem->id().value);
  CHECK(dbg->read<u64>(*rd_bytes) == 0);
  CHECK(dbg->read<u64>(*wr_bytes) == 0);
  REQUIRE_NOTHROW(mem->write(0, {program.data(), program.size()}, rw));
  CHECK(dbg->read<u64>(*rd_bytes) == 0);
  CHECK(program.size() != 0);
  CHECK(dbg->read<u64>(*wr_bytes) == program.size());
  const u16 init_sp = 0xFEED;
  const u16 end_pc = 0xDEAD;

  cpu->registers()->clear(0);
  cpu->csrs()->clear(0);
  cpu->write_register(Register::SP, init_sp);
  cpu->write_packed_csr(PepCSRBank::pack(true, false, true, false)); // NZVC = 1010

  REQUIRE_NOTHROW(cpu->clock_tick(PulseSchedule::PulseIndex{0}, 0));


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

TEST_CASE("Find register by name in HW debugger", "[scope:core][scope:core.dbg][kind:instr][arch:pep10]") {
  using Register = isa::Pep10::Register;
  using CSR = isa::Pep10::CSR;
  using MN = isa::Pep10::Mnemonic;
  inner_call<Register, CSR, MN>(PepISA3CPU::ISA::Pep10, MN::CALL);
}

TEST_CASE("A pointer-backed register must declare its storage's width",
          "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  auto [sys, mem, cpu] = make_cpu(PepISA3CPU::ISA::Pep10);
  auto *scan = sys->register_scan();
  u32 counter = 0;

  RegisterScan::Register r{};
  r.order = bits::hostOrder();
  r.target = mem->id();
  r.name = "counter";
  r.loc = &counter;

  // The visitors compare sizeof(T) against byte_width on every access, so a mismatch that expose() let through would
  // only surface as a throw out of the middle of some later read.
  r.byte_width = 2;
  CHECK_THROWS_AS(scan->expose(r), std::logic_error);

  r.byte_width = 4;
  REQUIRE_NOTHROW(scan->expose(r));
  counter = 0x1122'3344;
  CHECK(scan->read<u32>(*scan->find("counter")) == 0x1122'3344);
}

TEST_CASE("Register reads are host-order regardless of the register's order",
          "[scope:core][scope:core.dbg][kind:unit][arch:pep10]") {
  auto [sys, mem, cpu] = make_cpu(PepISA3CPU::ISA::Pep10);
  auto *scan = sys->register_scan();

  // The same 32-bit value laid out both ways in memory. read<I> reports host-order values, so both registers have to
  // read back as the same number. The Pep cores only ever expose big-endian registers, which is why the
  // little-endian case went unnoticed.
  constexpr u32 VALUE = 0x1122'3344;
  constexpr std::array<u8, 4> AS_BE{0x11, 0x22, 0x33, 0x44};
  constexpr std::array<u8, 4> AS_LE{0x44, 0x33, 0x22, 0x11};
  mem->write(0x10, {AS_BE.data(), AS_BE.size()}, rw);
  mem->write(0x20, {AS_LE.data(), AS_LE.size()}, rw);

  auto expose = [&](std::string name, bits::Order order, Address offset) {
    RegisterScan::Register r{};
    r.order = order;
    r.byte_width = 4;
    r.guest_access = RegisterScan::Register::ReadWrite;
    r.target = mem->id();
    r.loc = offset;
    r.name = std::move(name);
    scan->expose(r);
  };
  expose("BE_REG", bits::Order::BigEndian, 0x10);
  expose("LE_REG", bits::Order::LittleEndian, 0x20);

  CHECK(scan->read<u32>(*scan->find("BE_REG")) == VALUE);
  CHECK(scan->read<u32>(*scan->find("LE_REG")) == VALUE);
}
