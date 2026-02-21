#include <catch.hpp>
#include <cstdint>
#include "core/arch/riscv/isa/rvc.hpp"
#include "sim3/systems/notraced_riscv_isa3_system.hpp"

#include "sim3/systems/notraced_riscv_isa3_system/debug.hpp"
#include "testable_instruction.hpp"

TEST_CASE("RV32I", "[scope:sim][kind:int][arch:RV]") {

  static const std::array<uint32_t, 4> instructions = {
      0x00065637, // lui     a2,0x65
      0x000655b7, // lui     a1,0x65
      0x11612023, // sw      s6,256(sp)
      0x0b410b13, // addi    s6,sp,180
  };

  riscv::Machine<riscv::RISCV32> m{std::string_view{}, {.memory_max = 65536}};
  // install instructions
  m.cpu.init_execute_area(instructions.data(), 0x1000, 4 * instructions.size());
  m.cpu.jump(0x1000);

  // stack frame
  m.cpu.reg(riscv::REG_SP) = 0x120000 - 288;
  const uint32_t current_sp = m.cpu.reg(riscv::REG_SP);

  // execute LUI a2, 0x65000
  m.cpu.step_one();
  CHECK(m.cpu.reg(riscv::REG_ARG2) == 0x65000);
  // execute LUI a1, 0x65000
  m.cpu.step_one();
  CHECK(m.cpu.reg(riscv::REG_ARG1) == 0x65000);
  // execute SW  s6, [SP + 256]
  m.cpu.reg(22) = 0x12345678;
  m.cpu.step_one();
  CHECK(m.memory.template read<uint32_t>(current_sp + 256) == m.cpu.reg(22));
  // execute ADDI s6, [SP + 180]
  m.cpu.reg(22) = 0x0;
  m.cpu.step_one();
  CHECK(m.cpu.reg(22) == current_sp + 180);
}

TEST_CASE("RV32C", "[scope:sim][kind:int][arch:RV]") {
  riscv::Machine<uint32_t> machine{std::string_view{}, {.memory_max = 65536}};
  SECTION("C.SRLI") { // C.SRLI imm = [0, 31] CI_CODE(0b100, 0b01):
    for (int i = 0; i < 32; i++) {
      riscv::rv32c_instruction ci;
      ci.CA.opcode = 0b01;     // Quadrant 1
      ci.CA.funct6 = 0b100000; // ALU OP: SRLI
      ci.CAB.srd = 0x2;        // A0
      ci.CAB.imm04 = i;
      ci.CAB.imm5 = 0;

      const riscv::testable_insn<uint32_t> insn{
          .name = "C.SRLI", .bits = ci.whole, .reg = riscv::REG_ARG0, .index = i, .initial_value = 0xFFFFFFFF};
      bool b = validate<uint32_t>(machine, insn, [](auto &cpu, const auto &insn) -> bool {
        return cpu.reg(insn.reg) == (insn.initial_value >> insn.index);
      });
      CHECK(b);
    }
  }
  SECTION("C.SRAI") { // C.SRAI imm = [0, 31] CI_CODE(0b100, 0b01):

    for (int i = 0; i < 32; i++) {
      riscv::rv32c_instruction ci;
      ci.CA.opcode = 0b01;     // Quadrant 1
      ci.CA.funct6 = 0b100001; // ALU OP: SRAI
      ci.CAB.srd = 0x2;        // A0
      ci.CAB.imm04 = i;
      ci.CAB.imm5 = 0;

      const riscv::testable_insn<uint32_t> insn{
          .name = "C.SRAI", .bits = ci.whole, .reg = riscv::REG_ARG0, .index = i, .initial_value = 0xFFFFFFFF};
      bool b = validate<uint32_t>(
          machine, insn, [](auto &cpu, const auto &insn) -> bool { return cpu.reg(insn.reg) == insn.initial_value; });
      CHECK(b);
    }
  };
  SECTION("C.ANDI") { // C.ANDI imm = [-32, 31] CI_CODE(0b100, 0b01):
    for (int i = 0; i < 64; i++) {
      riscv::rv32c_instruction ci;
      ci.CA.opcode = 0b01;     // Quadrant 1
      ci.CA.funct6 = 0b100010; // ALU OP: ANDI
      ci.CAB.srd = 0x2;        // A0
      ci.CAB.imm04 = i & 31;
      ci.CAB.imm5 = i >> 5;

      const riscv::testable_insn<uint32_t> insn{
          .name = "C.ANDI", .bits = ci.whole, .reg = riscv::REG_ARG0, .index = i, .initial_value = 0xFFFFFFFF};
      bool b = validate<uint32_t>(machine, insn, [](auto &cpu, const auto &insn) -> bool {
        if (insn.index < 32) {
          return cpu.reg(insn.reg) == (insn.initial_value & insn.index);
        }
        return cpu.reg(insn.reg) == (insn.initial_value & (insn.index - 64));
      });
      CHECK(b);
    }
  }
  SECTION("C.SLLI") { // C.SLLI imm = [0, 31] CI_CODE(0b011, 0b10):
    for (int i = 0; i < 32; i++) {
      riscv::rv32c_instruction ci;
      ci.CI.opcode = 0b10; // Quadrant 1
      ci.CI.funct3 = 0x0;  // OP: SLLI
      ci.CI.rd = 0xA;      // A0
      ci.CI.imm1 = i;
      ci.CI.imm2 = 0;

      const riscv::testable_insn<uint32_t> insn{
          .name = "C.SLLI", .bits = ci.whole, .reg = riscv::REG_ARG0, .index = i, .initial_value = 0xA};
      bool b = validate<uint32_t>(machine, insn, [](auto &cpu, const auto &insn) -> bool {
        return cpu.reg(insn.reg) == (insn.initial_value << insn.index);
      });
      CHECK(b);
    }
  }
}

TEST_CASE("RV32 Custom Machine", "[scope:sim][kind:int][arch:RV][!throws]") {
  // this is a custom machine with very little virtual memory
  riscv::Machine<uint32_t> m2{std::string_view{}, {.memory_max = 65536}};

  // free the zero-page to reclaim 4k
  m2.memory.free_pages(0x0, riscv::Page::size());

  // fake a start at 0x1068
  const uint32_t entry_point = 0x1068;
  try {
    m2.cpu.jump(entry_point);
  } catch (...) {
  }
  try {
    m2.simulate();
  } catch (...) {
  }

  CHECK(m2.instruction_counter() == 0);
}

TEST_CASE("RV32 run exactly X instructions", "[scope:sim][kind:int][arch:RV]") {
  riscv::Machine<uint32_t> machine;

  std::array<uint32_t, 3> my_program{
      0x29a00513, //        li      a0,666
      0x05d00893, //        li      a7,93
      0xffdff06f, //        jr      -4
  };

  const uint32_t dst = 0x1000;
  machine.copy_to_guest(dst, &my_program[0], sizeof(my_program));
  machine.memory.set_page_attr(dst, riscv::Page::size(), {.read = false, .write = false, .exec = true});
  machine.cpu.jump(dst);

  // Step instruction by instruction using
  // a debugger.
  riscv::DebugMachine debugger{machine};
  debugger.verbose_instructions = true;

  debugger.simulate(3);
  REQUIRE(machine.cpu.reg(riscv::REG_ARG0) == 666);
  REQUIRE(machine.cpu.reg(riscv::REG_ARG7) == 93);
  REQUIRE(machine.instruction_counter() == 3);

  machine.cpu.reg(riscv::REG_ARG7) = 0;

  debugger.simulate(2);
  REQUIRE(machine.instruction_counter() == 5);
  REQUIRE(machine.cpu.reg(riscv::REG_ARG7) == 93);

  // Reset CPU registers and counter
  machine.cpu.registers() = {};
  machine.cpu.jump(dst);

  // Normal simulation
  machine.simulate<false>(2, 0u);
  REQUIRE(machine.instruction_counter() == 3);
  REQUIRE(machine.cpu.reg(riscv::REG_ARG7) == 93);

  machine.cpu.reg(riscv::REG_ARG7) = 0;

  machine.simulate<false>(2, machine.instruction_counter());
  REQUIRE(machine.instruction_counter() == 5);
  REQUIRE(machine.cpu.reg(riscv::REG_ARG7) == 93);
}
