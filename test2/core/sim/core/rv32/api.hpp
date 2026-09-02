#pragma once
#include "core/math/bitmanip/copy.hpp"
#include "core/sim/cores/cpu/rv32/rv_isa.hpp"
#include "core/sim/memory/ram/dense.hpp"
#include "core/sim/system.hpp"

namespace rv32_test {
inline const Operation rw{Operation::Type::Standard, Operation::Kind::data};

// First return value is the system (don't drop it!)
inline auto make_cpu() {
  using namespace bits;
  RV32CPU::Configuration cpu_cfg{Device::Configuration{
                                     .basename = "cpu",
                                     .compatible = RV32CPU::compatible,
                                 },
                                 "/memory"};
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
  auto *cpu = system->make_device<RV32CPU>(cpu_cfg, system.get());
  system->initialize();
  return std::make_tuple(std::move(system), mem, cpu);
}

// Load program into memory at address 0, respecting differences in host-guest endianness.
inline void load_program(Dense *mem, std::initializer_list<u32> words) {
  std::size_t i = 0;
  for (auto word : words) mem->write<u32, !bits::host_is_le>(i++ * sizeof(u32), word, rw);
}
} // namespace rv32_test
