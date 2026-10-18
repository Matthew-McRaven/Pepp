#pragma once
#include <algorithm>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include "core/arch/pepx/isa/pep10.hpp"
#include "core/arch/pepx/isa/pep9.hpp"
#include "core/sim/cores/cpu/pep/pep_isa.hpp"
#include "core/sim/cores/cpu/pep/pep_isa_instructions.hpp"
#include "core/sim/clocktree.hpp"
#include "core/sim/memory/ram/dense.hpp"
#include "core/sim/system.hpp"

// First return value is the system (don't drop it!)
inline auto make_cpu(PepISA3CPU::ISA isa = PepISA3CPU::ISA::Pep10) {
  using namespace bits;
  PepISA3CPU::Configuration cpu_cfg{Device::Configuration{
                                        .basename = "cpu",
                                        .compatible = PepISA3CPU::compatible,
                                    },
                                    isa, "/memory", "/clk"};
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
  pepp::IdealClock::Configuration clk_cfg{
      Device::Configuration{.basename = "clk", .compatible = pepp::IdealClock::compatible}, 1000};
  system->make_device<pepp::IdealClock>(clk_cfg);
  auto *cpu = system->make_device<PepISA3CPU>(cpu_cfg, system.get());
  system->initialize();
  return std::make_tuple(std::move(system), mem, cpu);
}

static const Operation rw{Operation::Type::Standard, Operation::Kind::data};

static inline u16 reg(PepISA3CPU *cpu, pep::Register reg) { return cpu->read_register(reg); };

static inline u16 csr(PepISA3CPU *cpu, pep::StatusBit csr) { return cpu->read_csr(csr); };

// What an opcode does in the CPU's ISA.
static inline pep::SharedOpBehavior behavior(PepISA3CPU::ISA isa, u8 opcode) {
  switch (isa) {
  case PepISA3CPU::ISA::Pep9: return pep::pep9::opcode_plane[opcode].opcode;
  case PepISA3CPU::ISA::Pep10: return pep::pep10::opcode_plane[opcode].opcode;
  default: throw std::invalid_argument("No opcode plane for ISA");
  }
}

// Base opcodes across all of the architecture's mnemonics
struct Opcodes {
  std::span<const pep::InstructionDefinition> rows;
  u8 operator()(std::string_view name) const {
    const auto row = std::ranges::find(rows, name, &pep::InstructionDefinition::mnemonic);
    if (row == rows.end()) throw std::invalid_argument("No mnemonic named " + std::string{name});
    return row->base;
  }
};
