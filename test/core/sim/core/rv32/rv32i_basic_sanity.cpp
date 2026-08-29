#include <bit>
#include <catch.hpp>
#include <cstdint>
#include "core/arch/riscv/isa/rvc.hpp"
#include "core/math/bitmanip/copy.hpp"
#include "core/sim/cores/cpu/rv32/rv_isa.hpp"
#include "core/sim/memory/ram/dense.hpp"
#include "core/sim/system.hpp"

namespace {
inline static const Operation rw{Operation::Type::Standard, Operation::Kind::data};
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
} // namespace

TEST_CASE("RV32I sanity tests", "[scope:core][scope:core.sim][kind:unit][arch:rv]") {

  static const std::array<uint32_t, 4> instructions = {
      0x00065637, // lui     a2,0x65
      0x000655b7, // lui     a1,0x65
      0x11612023, // sw      s6,256(sp)
      0x0b410b13, // addi    s6,sp,180
  };

  auto [sys, mem, cpu] = make_cpu();
  // Copy instructions in in LE-order. Real usecases would have assembled to u8s rather than u32s to avoid this step.
  for (std::size_t i = 0; i < instructions.size(); ++i)
    mem->write<uint32_t, !bits::host_is_le>(i * sizeof(uint32_t), instructions[i], rw);
  cpu->write_register(riscv::ABIReg::sp, 0xbee0);
  const uint32_t current_sp = cpu->read_register(riscv::ABIReg::sp);

  // execute LUI a2, 0x65000
  cpu->clock_tick(PulseSchedule::PulseIndex{(u64)0}, 0);
  CHECK(cpu->read_register(riscv::ABIReg::a2) == 0x65000);
  // execute LUI a1, 0x65000
  cpu->clock_tick(PulseSchedule::PulseIndex{(u64)1}, 1);
  CHECK(cpu->read_register(riscv::ABIReg::a1) == 0x65000);
  // execute SW  s6, [SP + 256]
  cpu->write_register(riscv::ABIReg::s6, 0x12345678);
  cpu->clock_tick(PulseSchedule::PulseIndex{(u64)2}, 2);
  const u32 val = mem->read<u32, !bits::host_is_le>(current_sp + 256, rw).second;
  CHECK(val == cpu->read_register(riscv::ABIReg::s6));
  // execute ADDI s6, [SP + 180]
  cpu->write_register(riscv::ABIReg::s6, 0x0);
  cpu->clock_tick(PulseSchedule::PulseIndex{(u64)3}, 3);
  CHECK(cpu->read_register(riscv::ABIReg::s6) == current_sp + 180);
}