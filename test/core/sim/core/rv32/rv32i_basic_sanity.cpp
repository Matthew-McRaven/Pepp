#include <bit>
#include <catch.hpp>
#include <cstdint>
#include "./api.hpp"

using namespace rv32_test;

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