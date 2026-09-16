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
#include <array>
#include <catch.hpp>

#include "core/arch/pep/isa/pep10.hpp"
#include "core/sim/api/loadable.hpp"
#include "core/sim/cores/cpu/pep/pep_isa.hpp"
#include "core/sim/loader.hpp"
#include "core/sim/memory/ram/dense.hpp"
#include "core/sim/system.hpp"

namespace {

// A Pep/10 core with one RAM behind it, as make_cpu does elsewhere.
auto make_cpu() {
  PepISA3CPU::Configuration cpu_cfg{
      Device::Configuration{.basename = "cpu", .compatible = PepISA3CPU::compatible},
      PepISA3CPU::ISA::Pep10,
      "/memory",
  };
  System::Configuration root_cfg{{.basename = "/", .compatible = System::compatible}};
  Dense::Configuration mem_cfg{
      Device::Configuration{.basename = "memory", .compatible = Dense::compatible},
      0x00,
      AddressSpan(0x0000, 0xffff),
  };
  auto system = std::make_unique<System>(root_cfg);
  auto *mem = system->make_device<Dense>(mem_cfg);
  auto *cpu = system->make_device<PepISA3CPU>(cpu_cfg, system.get());
  system->initialize();
  return std::make_tuple(std::move(system), mem, cpu);
}

const Operation app(Operation::Type::Application, Operation::Kind::data);

// The vectors are big-endian in memory, while the register bank is host-ordered.
void poke_be(Target *mem, Address at, u16 v) {
  const std::array<u8, 2> bytes{(u8)(v >> 8), (u8)(v & 0xFF)};
  mem->write(at, {bytes.data(), bytes.size()}, app);
}
const bool swap = bits::hostOrder() != bits::Order::BigEndian;

using MV = isa::Pep10::MemoryVectors;

} // namespace

TEST_CASE("Loader: initial register programming", "[scope:core][scope:core.sim][kind:int][arch:pep10]") {
  auto [sys, mem, cpu] = make_cpu();
  auto *scan = sys->register_scan();
  // Provide dummy values for the vectors so that we can check it the load actually occurs.
  poke_be(mem, (Address)MV::Dispatcher, 0xBEEF);
  poke_be(mem, (Address)MV::SystemStackPtr, 0xFAB0);
  const auto pc = *scan->find("/cpu:PC"), sp = *scan->find("/cpu:SP"), a = *scan->find("/cpu:A");

  SECTION("Load constants and from memory") {
    Loader loader(sys.get());
    REQUIRE(loader.set_register(a, 0x1234));
    REQUIRE(loader.copy_word(pc, mem->id(), (Address)MV::Dispatcher, swap));
    REQUIRE(loader.run());
    CHECK(loader.stop_cause() == tvm::StopCause::None);
    CHECK(scan->read<u16>(a) == 0x1234);
    CHECK(scan->read<u16>(pc) == 0xBEEF);
  }
  SECTION("Memory read at program execution time") {
    Loader loader(sys.get());
    REQUIRE(loader.copy_word(pc, mem->id(), (Address)MV::Dispatcher, swap));
    // Whatever the memory location held when the program was authored is ignored.
    poke_be(mem, (Address)MV::Dispatcher, 0x0BAD);
    REQUIRE(loader.run());
    CHECK(scan->read<u16>(pc) == 0x0BAD);
  }
  SECTION("Disallow invalid register references") {
    Loader loader(sys.get());
    const RegisterScan::RegisterRef nowhere{RegisterScan::Register::ID{0xFFFF}, RegisterScan::Register::Field::ID{0}};
    CHECK_FALSE(loader.set_register(nowhere, 1));
    CHECK_FALSE(loader.copy_word(nowhere, mem->id(), (Address)MV::Dispatcher));
  }
  SECTION("Pep/10 initializes SP/PC from memory vectors") {
    Loader loader(sys.get());
    auto *loadable = cpu->capability<Loadable>();
    REQUIRE(loadable != nullptr);
    loadable->register_core_init(loader);
    REQUIRE(loader.run());
    CHECK(scan->read<u16>(pc) == 0xBEEF);
    CHECK(scan->read<u16>(sp) == 0xFAB0);

    // test that we can re-run an existing program.
    poke_be(mem, (Address)MV::Dispatcher, 0x0100);
    REQUIRE(loader.run());
    CHECK(scan->read<u16>(pc) == 0x0100);
    CHECK(scan->read<u16>(sp) == 0xFAB0);
  }
}
