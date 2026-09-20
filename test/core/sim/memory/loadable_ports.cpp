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
#include <type_traits>

#include "core/sim/cores/cpu/pep/pep_isa.hpp"
#include "core/sim/cores/cpu/rv32/rv_isa.hpp"
#include "core/sim/memory/ram/dense.hpp"
#include "core/sim/clocktree.hpp"
#include "core/sim/system.hpp"

namespace {
using MemoryKind = Loadable::MemoryKind;

Dense::Configuration mem_cfg() {
  return {Device::Configuration{.basename = "memory", .compatible = Dense::compatible}, 0x00,
          AddressSpan(0x0000, 0xffff)};
}

// View that loader will see over the target. Make sure we can see instruction and data memories.
void check_unified(Device *cpu, Target *mem) {
  auto *loadable = cpu->capability<Loadable>();
  REQUIRE(loadable != nullptr);
  CHECK(loadable->port(MemoryKind::Instruction) == mem);
  // All memories are currently unified, with a shared space for code and data.
  CHECK(loadable->port(MemoryKind::Data) == loadable->port(MemoryKind::Instruction));
  CHECK(loadable->port(MemoryKind::MicrocodeROM) == nullptr);

  // Ensure the const route agrees as well.
  const Loadable *const_loadable = loadable;
  static_assert(std::is_same_v<decltype(const_loadable->port(MemoryKind::Instruction)), const Target *>);
  CHECK(const_loadable->port(MemoryKind::Instruction) == mem);
  CHECK(const_loadable->port(MemoryKind::Data) == const_loadable->port(MemoryKind::Instruction));
  CHECK(const_loadable->port(MemoryKind::MicrocodeROM) == nullptr);
}
} // namespace

TEST_CASE("Loadable ports", "[scope:core][scope:core.sim][kind:unit][arch:*]") {
  System::Configuration root_cfg{{.basename = "/", .compatible = System::compatible}};

  SECTION("Pep/10") {
    PepISA3CPU::Configuration cpu_cfg{
        Device::Configuration{.basename = "cpu", .compatible = PepISA3CPU::compatible},
        PepISA3CPU::ISA::Pep10,
        "/memory",
        "/clk",
    };
    auto sys = std::make_unique<System>(root_cfg);
    auto *mem = sys->make_device<Dense>(mem_cfg());
    pepp::IdealClock::Configuration clk_cfg{
        Device::Configuration{.basename = "clk", .compatible = pepp::IdealClock::compatible}, 1000};
    sys->make_device<pepp::IdealClock>(clk_cfg);
    auto *cpu = sys->make_device<PepISA3CPU>(cpu_cfg, sys.get());
    sys->initialize();
    check_unified(cpu, mem);
  }
  SECTION("RISC-V") {
    RV32CPU::Configuration cpu_cfg{
        Device::Configuration{.basename = "cpu", .compatible = RV32CPU::compatible},
        "/memory",
        "/clk",
    };
    auto sys = std::make_unique<System>(root_cfg);
    auto *mem = sys->make_device<Dense>(mem_cfg());
    pepp::IdealClock::Configuration clk_cfg{
        Device::Configuration{.basename = "clk", .compatible = pepp::IdealClock::compatible}, 1000};
    sys->make_device<pepp::IdealClock>(clk_cfg);
    auto *cpu = sys->make_device<RV32CPU>(cpu_cfg, sys.get());
    sys->initialize();
    check_unified(cpu, mem);
  }
}
