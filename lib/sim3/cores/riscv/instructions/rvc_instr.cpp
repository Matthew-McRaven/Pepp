/*
 * Copyright (c) 2025-2026 J. Stanley Warford, Matthew McRaven
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
 *
 * Copyright (c) 2024, Alf-André Walla
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS”
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * You should have received a copy of the BSD 3-clause license
 * along with this program. If not, see
 * <https://opensource.org/license/bsd-3-clause>
 */
#include "./rvc_instr.hpp"
#include <inttypes.h>
#include "../notraced_cpu.hpp"
#include "core/arch/riscv/isa/rv_base.hpp"
#include "core/arch/riscv/isa/rv_types.hpp"
#include "core/arch/riscv/isa/rvc.hpp"
#include "instr_helpers.hpp"
#include "sim3/common_macros.hpp"
#include "sim3/systems/notraced_riscv_isa3_system.hpp"

namespace riscv {
template <AddressType address_t> RVINSTR_ATTR void C0_ADDI4SPN_handler(CPU<address_t> &cpu, instruction_format instr) {
  const auto ciw = instr.as_compressed<InstructionCIW>();
  cpu.cireg(ciw.srd) = cpu.reg(REG_SP) + ciw.offset();
};
template <AddressType address_t>
RVPRINTR_ATTR int C0_ADDI4SPN_printer(char *buffer, size_t len, const CPU<address_t> &cpu, instruction_format instr) {
  const auto ciw = instr.as_compressed<InstructionCIW>();
  if (UNLIKELY(instr.low16() == 0)) {
    return snprintf(buffer, len, "INVALID: All zeroes");
  }
  return snprintf(buffer, len, "C.ADDI4SPN %s, SP+%u (0x%" PRIx64 ")", RISCV::ciname(ciw.srd), ciw.offset(),
                  uint64_t(cpu.reg(REG_SP) + ciw.offset()));
};

// LW, LD, LQ, FLW, FLD
template <AddressType address_t> RVINSTR_ATTR void C0_REG_FLD_handler(CPU<address_t> &cpu, instruction_format instr) {
  const auto csd = instr.as_compressed<InstructionCSD>();
  const auto cl = instr.as_compressed<InstructionCL>();
  auto address = cpu.cireg(cl.srs1) + csd.offset8();
  cpu.ciflp(cl.srd).load_u64(cpu.machine().memory.template read<uint64_t>(address));
};
template <AddressType address_t>
RVPRINTR_ATTR int C0_REG_FLD_printer(char *buffer, size_t len, const CPU<address_t> &cpu, instruction_format instr) {
  const auto cl = instr.as_compressed<InstructionCL>();
  static const std::array<const char *, 4> f3 = {"???", "FLD", "LW", RVIS64BIT(cpu) ? "LD" : "FLW"};
  return snprintf(buffer, len, "C.%s %s, [%s+%u = 0x%lX]", f3[cl.funct3], RISCV::ciname(cl.srd),
                  RISCV::ciname(cl.srs1), cl.offset(), (long)cpu.cireg(cl.srs1) + cl.offset());
};
template <AddressType address_t> RVINSTR_ATTR void C0_REG_LW_handler(CPU<address_t> &cpu, instruction_format instr) {
  const auto cl = instr.as_compressed<InstructionCL>();
  auto address = cpu.cireg(cl.srs1) + cl.offset();
  cpu.cireg(cl.srd) = (int32_t)cpu.machine().memory.template read<uint32_t>(address);
};
template <AddressType address_t> RVINSTR_ATTR void C0_REG_LD_handler(CPU<address_t> &cpu, instruction_format instr) {
  const auto csd = instr.as_compressed<InstructionCSD>();
  auto address = cpu.cireg(csd.srs1) + csd.offset8();
  cpu.cireg(csd.srs2) = (int64_t)cpu.machine().memory.template read<uint64_t>(address);
};
template <AddressType address_t> RVINSTR_ATTR void C0_REG_FLW_handler(CPU<address_t> &cpu, instruction_format instr) {
  const auto cl = instr.as_compressed<InstructionCL>();
  auto address = cpu.cireg(cl.srs1) + cl.offset();
  cpu.ciflp(cl.srd).load_u32(cpu.machine().memory.template read<uint32_t>(address));
};
// SW, SD, SQ, FSW, FSD
template <AddressType address_t> RVINSTR_ATTR void C0_REG_FSD_handler(CPU<address_t> &cpu, instruction_format instr) {
  const auto csd = instr.as_compressed<InstructionCSD>();
  const auto address = cpu.cireg(csd.srs1) + csd.offset8();
  const auto value = cpu.ciflp(csd.srs2).i64;
  cpu.machine().memory.template write<uint64_t>(address, value);
};
template <AddressType address_t>
RVPRINTR_ATTR int C0_REG_FSD_printer(char *buffer, size_t len, const CPU<address_t> &cpu, instruction_format instr) {
  const auto csd = instr.as_compressed<InstructionCSD>();
  const auto cs = instr.as_compressed<InstructionCS>();
  static const std::array<const char *, 4> f3_32 = {"Reserved instruction", "FSD", "SW", "FSW"};
  static const std::array<const char *, 4> f3_64 = {"Reserved instruction", "FSD", "SW", "SD"};
  static const auto &f3 = RVIS64BIT(cpu) ? f3_64 : f3_32;
  const int offset = (cs.funct3 == 0x7) ? cs.offset4() : csd.offset8();
  if (cs.funct3 == 0x6 || (cs.funct3 == 0x07 && RVIS64BIT(cpu)))
    return snprintf(buffer, len, "C.%s %s, [%s%+d]", f3[cs.funct3 - 4], RISCV::ciname(cs.srs2),
                    RISCV::ciname(cs.srs1), offset);
  else
    return snprintf(buffer, len, "C.%s %s, [%s%+d]", f3[cs.funct3 - 4], RISCV::ciflp(cs.srs2),
                    RISCV::ciname(cs.srs1), offset);
};
template <AddressType address_t> RVINSTR_ATTR void C0_REG_SW_handler(CPU<address_t> &cpu, instruction_format instr) {
  const auto cs = instr.as_compressed<InstructionCS>();
  const auto address = cpu.cireg(cs.srs1) + cs.offset4();
  const auto value = cpu.cireg(cs.srs2);
  cpu.machine().memory.template write<uint32_t>(address, value);
};
template <AddressType address_t> RVINSTR_ATTR void C0_REG_SD_handler(CPU<address_t> &cpu, instruction_format instr) {
  const auto csd = instr.as_compressed<InstructionCSD>();
  const auto address = cpu.cireg(csd.srs1) + csd.offset8();
  const auto value = cpu.cireg(csd.srs2);
  cpu.machine().memory.template write<uint64_t>(address, value);
};
template <AddressType address_t> RVINSTR_ATTR void C0_REG_FSW_handler(CPU<address_t> &cpu, instruction_format instr) {
  const auto cs = instr.as_compressed<InstructionCS>();
  const auto address = cpu.cireg(cs.srs1) + cs.offset4();
  const auto value = cpu.ciflp(cs.srs2).i32[0];
  cpu.machine().memory.template write<uint32_t>(address, value);
};

template <AddressType address_t> RVINSTR_ATTR void C1_ADDI_handler(CPU<address_t> &cpu, instruction_format instr) {
  const auto ci = instr.as_compressed<InstructionCI>();
  // C.ADDI (non-hint, not NOP)
  cpu.reg(ci.rd) += ci.signed_imm();
};
template <AddressType address_t>
RVPRINTR_ATTR int C1_ADDI_printer(char *buffer, size_t len, const CPU<address_t> &, instruction_format instr) {
  const auto ci = instr.as_compressed<InstructionCI>();
  if (ci.rd != 0) {
    return snprintf(buffer, len, "C.ADDI %s, %" PRId32, RISCV::regname(ci.rd), ci.signed_imm());
  }
  if (ci.imm1 != 0 || ci.imm2 != 0) return snprintf(buffer, len, "C.HINT");
  return snprintf(buffer, len, "C.NOP");
};

template <AddressType address_t> RVINSTR_ATTR void C1_JAL_handler(CPU<address_t> &cpu, instruction_format instr) {
  const auto cj = instr.as_compressed<InstructionCJ>();
  cpu.reg(REG_RA) = cpu.pc() + 2; // return instruction
  const auto address = cpu.pc() + cj.signed_imm();
  cpu.jump(address - 2);
  if constexpr (verbose_branches_enabled) {
    printf(">>> CALL 0x%lX <-- %s = 0x%lX\n", (long)address, RISCV::regname(REG_RA), (long)cpu.reg(REG_RA));
  }
};
template <AddressType address_t>
RVPRINTR_ATTR int C1_JAL_printer(char *buffer, size_t len, const CPU<address_t> &cpu, instruction_format instr) {
  const auto cj = instr.as_compressed<InstructionCJ>();
  return snprintf(buffer, len, "C.JAL %s, PC%+" PRId32 " (0x%" PRIX64 ")", RISCV::regname(REG_RA), cj.signed_imm(),
                  uint64_t(cpu.pc() + cj.signed_imm()));
};

template <AddressType address_t> RVINSTR_ATTR void C1_ADDIW_handler(CPU<address_t> &cpu, instruction_format instr) {
  const auto ci = instr.as_compressed<InstructionCI>();
  // C.ADDIW rd, imm[5:0]
  const uint32_t src = cpu.reg(ci.rd);
  cpu.reg(ci.rd) = (int32_t)(src + ci.signed_imm());
};
template <AddressType address_t>
RVPRINTR_ATTR int C1_ADDIW_printer(char *buffer, size_t len, const CPU<address_t> &, instruction_format instr) {
  const auto ci = instr.as_compressed<InstructionCI>();
  return snprintf(buffer, len, "C.ADDIW %s, %+" PRId32, RISCV::regname(ci.rd), ci.signed_imm());
};

template <AddressType address_t> RVINSTR_ATTR void C1_LI_handler(CPU<address_t> &cpu, instruction_format instr) {
  const auto ci = instr.as_compressed<InstructionCI>();
  // C.LI rd, imm[5:0]
  cpu.reg(ci.rd) = ci.signed_imm();
};
template <AddressType address_t>
RVPRINTR_ATTR int C1_LI_printer(char *buffer, size_t len, const CPU<address_t> &, instruction_format instr) {
  const auto ci = instr.as_compressed<InstructionCI>();
  return snprintf(buffer, len, "C.LI %s, %+" PRId32, RISCV::regname(ci.rd), ci.signed_imm());
};

template <AddressType address_t> RVINSTR_ATTR void C1_ADDI16SP_handler(CPU<address_t> &cpu, instruction_format instr) {
  const auto ci16 = instr.as_compressed<InstructionCI16>();
  // C.ADDI16SP rd, imm[17:12]
  cpu.reg(REG_SP) += ci16.signed_imm();
};
template <AddressType address_t>
RVPRINTR_ATTR int C1_ADDI16SP_printer(char *buffer, size_t len, const CPU<address_t> &, instruction_format instr) {
  const auto ci16 = instr.as_compressed<InstructionCI16>();
  const auto ci = instr.as_compressed<InstructionCI>();
  if (ci.rd != 0 && ci.rd != 2) {
    return snprintf(buffer, len, "C.LUI %s, 0x%" PRIX32, RISCV::regname(ci.rd), ci.upper_imm());
  } else if (ci.rd == 2) {
    return snprintf(buffer, len, "C.ADDI16SP %s, %+" PRId32, RISCV::regname(ci.rd), ci16.signed_imm());
  }
  return snprintf(buffer, len, "C.LUI (Invalid values)");
};
template <AddressType address_t> RVINSTR_ATTR void C1_LUI_handler(CPU<address_t> &cpu, instruction_format instr) {
  const auto ci = instr.as_compressed<InstructionCI>();
  // LUI rd, imm[17:12] (sign-extended)
  cpu.reg(ci.rd) = (int32_t)ci.upper_imm();
};
template <AddressType address_t> RVINSTR_ATTR void C1_ALU_OPS_handler(CPU<address_t> &cpu, instruction_format instr) {
  const auto cab = instr.as_compressed<InstructionCAB>();
  const auto ca = instr.as_compressed<InstructionCA>();
  auto &dst = cpu.cireg(ca.srd);
  switch (ca.funct6 & 0x3) {
  case 0: // C.SRLI
    if constexpr (RVIS64BIT(cpu)) {
      dst = dst >> cab.shift64_imm();
    } else {
      dst = dst >> cab.shift_imm();
    }
    return;
  case 1: // C.SRAI (preserve sign)
    dst = (RVSIGNTYPE(cpu))dst >> (cab.shift64_imm() & (RVXLEN(cpu) - 1));
    return;
  case 2: // C.ANDI
    dst = dst & cab.signed_imm();
    return;
  case 3: // more ops
    const auto &src = cpu.cireg(ca.srs2);
    switch (ca.funct2 | (ca.funct6 & 0x4)) {
    case 0: // C.SUB
      dst = dst - src;
      return;
    case 1: // C.XOR
      dst = dst ^ src;
      return;
    case 2: // C.OR
      dst = dst | src;
      return;
    case 3: // C.AND
      dst = dst & src;
      return;
    case 4: // C.SUBW
      if constexpr (RVIS64BIT(cpu)) {
        dst = (int32_t)((uint32_t)dst - (uint32_t)src);
        return;
      }
      break;
    case 5: // C.ADDW
      if constexpr (RVIS64BIT(cpu)) {
        dst = (int32_t)((uint32_t)dst + (uint32_t)src);
        return;
      }
      break;
    default: break;
    }
  }
  cpu.trigger_exception(ILLEGAL_OPCODE);
};
template <AddressType address_t>
RVPRINTR_ATTR int C1_ALU_OPS_printer(char *buffer, size_t len, const CPU<address_t> &cpu, instruction_format instr) {
  const auto cab = instr.as_compressed<InstructionCAB>();
  const auto ca = instr.as_compressed<InstructionCA>();
  if ((ca.funct6 & 0x3) < 2) {
    static const std::array<const char *, 2> f3 = {"SRLI", "SRAI"};
    return snprintf(buffer, len, "C.%s %s, %+d", f3[ca.funct6 & 0x3], RISCV::ciname(cab.srd),
                    RVIS64BIT(cpu) ? cab.shift64_imm() : cab.shift_imm());
  } else if ((ca.funct6 & 0x3) == 2) {
    return snprintf(buffer, len, "C.ANDI %s, %+" PRId32, RISCV::ciname(cab.srd), cab.signed_imm());
  }
  const int op = ca.funct2 | (ca.funct6 & 0x4);
  static const std::array<const char *, 8> f3 = {"SUB", "XOR", "OR", "AND", "SUBW", "ADDW", "RESV", "RESV"};

  return snprintf(buffer, len, "C.%s %s, %s", f3[op], RISCV::ciname(ca.srd), RISCV::ciname(ca.srs2));
};

template <AddressType address_t> RVINSTR_ATTR void C1_JUMP_handler(CPU<address_t> &cpu, instruction_format instr) {
  const auto cj = instr.as_compressed<InstructionCJ>();
  cpu.jump(cpu.pc() + cj.signed_imm() - 2);
  if constexpr (verbose_branches_enabled) {
    printf(">>> C.JMP 0x%lX\n", (long)cpu.pc() + 2);
  }
};
template <AddressType address_t>
RVPRINTR_ATTR int C1_JUMP_printer(char *buffer, size_t len, const CPU<address_t> &cpu, instruction_format instr) {
  const auto cj = instr.as_compressed<InstructionCJ>();
  return snprintf(buffer, len, "C.JMP 0x%" PRIX64, uint64_t(cpu.pc() + cj.signed_imm()));
};
template <AddressType address_t> RVINSTR_ATTR void C1_BEQZ_handler(CPU<address_t> &cpu, instruction_format instr) {
  const auto cb = instr.as_compressed<InstructionCB>();
  // condition: register equals zero
  if (cpu.cireg(cb.srs1) == 0) {
    // branch taken
    cpu.jump(cpu.pc() + cb.signed_imm() - 2);
    if constexpr (verbose_branches_enabled) {
      printf(">>> BRANCH jump to 0x%" PRIX64 "\n", uint64_t(cpu.pc() + 2));
    }
  }
};
template <AddressType address_t>
RVPRINTR_ATTR int C1_BEQZ_printer(char *buffer, size_t len, const CPU<address_t> &cpu, instruction_format instr) {
  const auto cb = instr.as_compressed<InstructionCB>();
  return snprintf(buffer, len, "C.BEQZ %s, PC%+" PRId32 " (0x%" PRIX64 ")", RISCV::ciname(cb.srs1),
                  cb.signed_imm(), uint64_t(cpu.pc() + cb.signed_imm()));
};

template <AddressType address_t> RVINSTR_ATTR void C1_BNEZ_handler(CPU<address_t> &cpu, instruction_format instr) {
  const auto cb = instr.as_compressed<InstructionCB>();
  // condition: register not-equal zero
  if (cpu.cireg(cb.srs1) != 0) {
    // branch taken
    cpu.jump(cpu.pc() + cb.signed_imm() - 2);
    if constexpr (verbose_branches_enabled) {
      printf(">>> BRANCH jump to 0x%" PRIX64 "\n", (uint64_t)(cpu.pc() + 2));
    }
  }
};
template <AddressType address_t>
RVPRINTR_ATTR int C1_BNEZ_printer(char *buffer, size_t len, const CPU<address_t> &cpu, instruction_format instr) {
  const auto cb = instr.as_compressed<InstructionCB>();
  return snprintf(buffer, len, "C.BNEZ %s, PC%+" PRId32 " (0x%" PRIX64 ")", RISCV::ciname(cb.srs1),
                  cb.signed_imm(), uint64_t(cpu.pc() + cb.signed_imm()));
};

// C.SLLI, LWSP, LDSP, LQSP, FLWSP, FLDSP
template <AddressType address_t>
RVPRINTR_ATTR int C2_SLLI_printer(char *buffer, size_t len, const CPU<address_t> &cpu, instruction_format instr) {
  const auto cifld = instr.as_compressed<InstructionCIFLD>();
  const auto ci2 = instr.as_compressed<InstructionCI2>();
  const auto ci = instr.as_compressed<InstructionCI>();
  if (ci2.funct3 == 0x0 && ci2.rd != 0) {
    return snprintf(buffer, len, "C.SLLI %s, %u", RISCV::regname(ci.rd),
                    (RVIS64BIT(cpu)) ? ci.shift64_imm() : ci.shift_imm());
  } else if (ci2.rd != 0) {
    static const std::array<const char *, 4> f3 = {"???", "FLDSP", "LWSP", "FLWSP"};
    const char *regname = (ci2.funct3 & 1) ? RISCV::flpname(ci2.rd) : RISCV::regname(ci2.rd);
    auto address = (ci2.funct3 != 0x1) ? cpu.reg(REG_SP) + ci2.offset() : cpu.reg(REG_SP) + cifld.offset();
    return snprintf(buffer, len, "C.%s %s, [SP+%u] (0x%" PRIX64 ")", f3[ci2.funct3], regname, ci2.offset(),
                    uint64_t(address));
  }
  return snprintf(buffer, len, "C.HINT %s", RISCV::regname(ci2.rd));
};
template <AddressType address_t> RVINSTR_ATTR void C2_SLLI_handler(CPU<address_t> &cpu, instruction_format instr) {
  const auto ci = instr.as_compressed<InstructionCI>();
  if constexpr (RVIS64BIT(cpu)) {
    cpu.reg(ci.rd) <<= ci.shift64_imm();
  } else {
    cpu.reg(ci.rd) <<= ci.shift_imm();
  }
};
template <AddressType address_t> RVINSTR_ATTR void C2_FLDSP_handler(CPU<address_t> &cpu, instruction_format instr) {
  const auto cifld = instr.as_compressed<InstructionCIFLD>();
  auto address = cpu.reg(REG_SP) + cifld.offset();
  auto &dst = cpu.registers().getfl(cifld.rd);
  dst.load_u64(cpu.machine().memory.template read<uint64_t>(address));
};
template <AddressType address_t> RVINSTR_ATTR void C2_LWSP_handler(CPU<address_t> &cpu, instruction_format instr) {
  const auto ci2 = instr.as_compressed<InstructionCI2>();
  auto address = cpu.reg(REG_SP) + ci2.offset();
  cpu.reg(ci2.rd) = (int32_t)cpu.machine().memory.template read<uint32_t>(address);
};
template <AddressType address_t> RVINSTR_ATTR void C2_LDSP_handler(CPU<address_t> &cpu, instruction_format instr) {
  const auto cifld = instr.as_compressed<InstructionCIFLD>();
  auto address = cpu.reg(REG_SP) + cifld.offset();
  cpu.reg(cifld.rd) = (int64_t)cpu.machine().memory.template read<uint64_t>(address);
};
template <AddressType address_t> RVINSTR_ATTR void C2_FLWSP_handler(CPU<address_t> &cpu, instruction_format instr) {
  const auto ci2 = instr.as_compressed<InstructionCI2>();
  auto address = cpu.reg(REG_SP) + ci2.offset();
  auto &dst = cpu.registers().getfl(ci2.rd);
  dst.load_u32(cpu.machine().memory.template read<uint32_t>(address));
};
// SWSP, SDSP, SQSP, FSWSP, FSDSP
template <AddressType address_t>
RVPRINTR_ATTR int C2_FSDSP_printer(char *buffer, size_t len, const CPU<address_t> &cpu, instruction_format instr) {
  const auto css = instr.as_compressed<InstructionCSS>();
  static const std::array<const char *, 4> f3 = {"XXX", "FSDSP", "SWSP", RVIS64BIT(cpu) ? "SDSP" : "FSWSP"};
  auto address = cpu.reg(REG_SP) + css.offset(4);
  return snprintf(buffer, len, "C.%s [SP%+d], %s (0x%lX)", f3[css.funct3 - 4], css.offset(4),
                  RISCV::regname(css.rs2), (long)address);
};
template <AddressType address_t> RVINSTR_ATTR void C2_FSDSP_handler(CPU<address_t> &cpu, instruction_format instr) {
  const auto csfsd = instr.as_compressed<InstructionCSFSD>();
  auto addr = cpu.reg(REG_SP) + csfsd.offset();
  uint64_t value = cpu.registers().getfl(csfsd.rs2).i64;
  cpu.machine().memory.template write<uint64_t>(addr, value);
};
template <AddressType address_t> RVINSTR_ATTR void C2_SWSP_handler(CPU<address_t> &cpu, instruction_format instr) {
  const auto css = instr.as_compressed<InstructionCSS>();
  auto addr = cpu.reg(REG_SP) + css.offset(4);
  uint32_t value = cpu.reg(css.rs2);
  cpu.machine().memory.template write<uint32_t>(addr, value);
};
template <AddressType address_t> RVINSTR_ATTR void C2_FSWSP_handler(CPU<address_t> &cpu, instruction_format instr) {
  const auto css = instr.as_compressed<InstructionCSS>();
  auto addr = cpu.reg(REG_SP) + css.offset(4);
  uint32_t value = cpu.registers().getfl(css.rs2).i32[0];
  cpu.machine().memory.template write<uint32_t>(addr, value);
};
template <AddressType address_t> RVINSTR_ATTR void C2_SDSP_handler(CPU<address_t> &cpu, instruction_format instr) {
  const auto csfsd = instr.as_compressed<InstructionCSFSD>();
  auto addr = cpu.reg(REG_SP) + csfsd.offset();
  auto value = cpu.reg(csfsd.rs2);
  cpu.machine().memory.template write<uint64_t>(addr, value);
};
template <AddressType address_t>
RVPRINTR_ATTR int C2_SDSP_printer(char *buffer, size_t len, const CPU<address_t> &cpu, instruction_format instr) {
  const auto csfsd = instr.as_compressed<InstructionCSFSD>();
  const auto css = instr.as_compressed<InstructionCSS>();
  auto address = cpu.reg(REG_SP) + csfsd.offset();
  return snprintf(buffer, len, "C.SDSP [SP%+d], %s (0x%lX)", csfsd.offset(), RISCV::regname(css.rs2),
                  (long)address);
};

// C.JR, C.MV, C.JALR, C.ADD
template <AddressType address_t>
RVPRINTR_ATTR int C2_JR_printer(char *buffer, size_t len, const CPU<address_t> &cpu, instruction_format instr) {
  const auto cr = instr.as_compressed<InstructionCR>();
  const bool topbit = instr.low16() & (1 << 12);
  if (!topbit && cr.rs2 == 0 && cr.rd != 0) {
    if (cr.rd == REG_RA) return snprintf(buffer, len, "C.RET");
    return snprintf(buffer, len, "C.JR %s", RISCV::regname(cr.rd));
  } else if (!topbit && cr.rs2 != 0 && cr.rd != 0)
    return snprintf(buffer, len, "C.MV %s, %s", RISCV::regname(cr.rd), RISCV::regname(cr.rs2));
  else if (topbit && cr.rd != 0 && cr.rs2 == 0)
    return snprintf(buffer, len, "C.JALR RA, %s (0x%lX)", RISCV::regname(cr.rd), (long)cpu.reg(cr.rd));
  else if (cr.rd != 0)
    return snprintf(buffer, len, "C.ADD %s, %s + %s", RISCV::regname(cr.rd), RISCV::regname(cr.rd),
                    RISCV::regname(cr.rs2));
  else if (topbit && cr.rd == 0 && cr.rs2 == 0) return snprintf(buffer, len, "C.EBREAK");
  return snprintf(buffer, len, "C.HINT");
};
template <AddressType address_t> RVINSTR_ATTR void C2_JR_handler(CPU<address_t> &cpu, instruction_format instr) {
  const auto cr = instr.as_compressed<InstructionCR>();
  cpu.jump(cpu.reg(cr.rd) - 2);
  if constexpr (verbose_branches_enabled) {
    printf(">>> RET 0x%lX <-- %s = 0x%lX\n", (long)cpu.pc(), RISCV::regname(cr.rd), (long)cpu.reg(cr.rd));
  }
};
template <AddressType address_t> RVINSTR_ATTR void C2_JALR_handler(CPU<address_t> &cpu, instruction_format instr) {
  const auto cr = instr.as_compressed<InstructionCR>();
  cpu.reg(REG_RA) = cpu.pc() + 0x2;
  cpu.jump(cpu.reg(cr.rd) - 2);
  if constexpr (verbose_branches_enabled) {
    printf(">>> C.JAL RA, 0x%lX <-- %s = 0x%lX\n", (long)cpu.reg(REG_RA) - 2, RISCV::regname(cr.rd),
           (long)cpu.reg(cr.rd));
  }
};
template <AddressType address_t> RVINSTR_ATTR void C2_MV_handler(CPU<address_t> &cpu, instruction_format instr) {
  const auto cr = instr.as_compressed<InstructionCR>();
  cpu.reg(cr.rd) = cpu.reg(cr.rs2);
};
template <AddressType address_t> RVINSTR_ATTR void C2_ADD_handler(CPU<address_t> &cpu, instruction_format instr) {
  const auto cr = instr.as_compressed<InstructionCR>();
  cpu.reg(cr.rd) += cpu.reg(cr.rs2);
};
template <AddressType address_t> RVINSTR_COLDATTR void C2_EBREAK_handler(CPU<address_t> &cpu, instruction_format) {
  cpu.machine().ebreak();
};

} // namespace riscv

const riscv::Instruction<uint32_t> instr32i_C0_ADDI4SPN{riscv::C0_ADDI4SPN_handler, riscv::C0_ADDI4SPN_printer};
const riscv::Instruction<uint32_t> instr32i_C0_REG_FLD{riscv::C0_REG_FLD_handler, riscv::C0_REG_FLD_printer};
const riscv::Instruction<uint32_t> instr32i_C0_REG_LW{riscv::C0_REG_LW_handler, riscv::C0_REG_FLD_printer};
const riscv::Instruction<uint32_t> instr32i_C0_REG_FLW{riscv::C0_REG_FLW_handler, riscv::C0_REG_FLD_printer};
const riscv::Instruction<uint32_t> instr32i_C0_REG_FSD{riscv::C0_REG_FSD_handler, riscv::C0_REG_FSD_printer};
const riscv::Instruction<uint32_t> instr32i_C0_REG_SW{riscv::C0_REG_SW_handler, riscv::C0_REG_FSD_printer};
const riscv::Instruction<uint32_t> instr32i_C0_REG_SD{riscv::C0_REG_SD_handler, riscv::C0_REG_FSD_printer};
const riscv::Instruction<uint32_t> instr32i_C0_REG_FSW{riscv::C0_REG_FSW_handler, riscv::C0_REG_FSD_printer};
const riscv::Instruction<uint32_t> instr32i_C1_ADDI{riscv::C1_ADDI_handler, riscv::C1_ADDI_printer};
const riscv::Instruction<uint32_t> instr32i_C1_JAL{riscv::C1_JAL_handler, riscv::C1_JAL_printer};
const riscv::Instruction<uint32_t> instr32i_C1_ADDIW{riscv::C1_ADDIW_handler, riscv::C1_ADDIW_printer};
const riscv::Instruction<uint32_t> instr32i_C1_LI{riscv::C1_LI_handler, riscv::C1_LI_printer};
const riscv::Instruction<uint32_t> instr32i_C1_ADDI16SP{riscv::C1_ADDI16SP_handler, riscv::C1_ADDI16SP_printer};
const riscv::Instruction<uint32_t> instr32i_C1_LUI{riscv::C1_LUI_handler, riscv::C1_ADDI16SP_printer};
const riscv::Instruction<uint32_t> instr32i_C1_ALU_OPS{riscv::C1_ALU_OPS_handler, riscv::C1_ALU_OPS_printer};
const riscv::Instruction<uint32_t> instr32i_C1_JUMP{riscv::C1_JUMP_handler, riscv::C1_JUMP_printer};
const riscv::Instruction<uint32_t> instr32i_C1_BEQZ{riscv::C1_BEQZ_handler, riscv::C1_BEQZ_printer};
const riscv::Instruction<uint32_t> instr32i_C1_BNEZ{riscv::C1_BNEZ_handler, riscv::C1_BNEZ_printer};
const riscv::Instruction<uint32_t> instr32i_C2_SLLI{riscv::C2_SLLI_handler, riscv::C2_SLLI_printer};
const riscv::Instruction<uint32_t> instr32i_C2_FLDSP{riscv::C2_FLDSP_handler, riscv::C2_SLLI_printer};
const riscv::Instruction<uint32_t> instr32i_C2_LWSP{riscv::C2_LWSP_handler, riscv::C2_SLLI_printer};
const riscv::Instruction<uint32_t> instr32i_C2_LDSP{riscv::C2_LDSP_handler, riscv::C2_SLLI_printer};
const riscv::Instruction<uint32_t> instr32i_C2_FLWSP{riscv::C2_FLWSP_handler, riscv::C2_SLLI_printer};
const riscv::Instruction<uint32_t> instr32i_C2_FSDSP{riscv::C2_FSDSP_handler, riscv::C2_FSDSP_printer};
const riscv::Instruction<uint32_t> instr32i_C2_SWSP{riscv::C2_SWSP_handler, riscv::C2_FSDSP_printer};
const riscv::Instruction<uint32_t> instr32i_C2_SDSP{riscv::C2_SDSP_handler, riscv::C2_SDSP_printer};
const riscv::Instruction<uint32_t> instr32i_C2_FSWSP{riscv::C2_FSWSP_handler, riscv::C2_FSDSP_printer};
const riscv::Instruction<uint32_t> instr32i_C2_JR{riscv::C2_JR_handler, riscv::C2_JR_printer};
const riscv::Instruction<uint32_t> instr32i_C2_JALR{riscv::C2_JALR_handler, riscv::C2_JR_printer};
const riscv::Instruction<uint32_t> instr32i_C2_MV{riscv::C2_MV_handler, riscv::C2_JR_printer};
const riscv::Instruction<uint32_t> instr32i_C2_ADD{riscv::C2_ADD_handler, riscv::C2_JR_printer};
const riscv::Instruction<uint32_t> instr32i_C2_EBREAK{riscv::C2_EBREAK_handler, riscv::C2_JR_printer};

const riscv::Instruction<uint64_t> instr64i_C0_ADDI4SPN{riscv::C0_ADDI4SPN_handler, riscv::C0_ADDI4SPN_printer};
const riscv::Instruction<uint64_t> instr64i_C0_REG_FLD{riscv::C0_REG_FLD_handler, riscv::C0_REG_FLD_printer};
const riscv::Instruction<uint64_t> instr64i_C0_REG_LW{riscv::C0_REG_LW_handler, riscv::C0_REG_FLD_printer};
const riscv::Instruction<uint64_t> instr64i_C0_REG_LD{riscv::C0_REG_LD_handler, riscv::C0_REG_FLD_printer};
const riscv::Instruction<uint64_t> instr64i_C0_REG_FLW{riscv::C0_REG_FLW_handler, riscv::C0_REG_FLD_printer};
const riscv::Instruction<uint64_t> instr64i_C0_REG_FSD{riscv::C0_REG_FSD_handler, riscv::C0_REG_FSD_printer};
const riscv::Instruction<uint64_t> instr64i_C0_REG_SW{riscv::C0_REG_SW_handler, riscv::C0_REG_FSD_printer};
const riscv::Instruction<uint64_t> instr64i_C0_REG_SD{riscv::C0_REG_SD_handler, riscv::C0_REG_FSD_printer};
const riscv::Instruction<uint64_t> instr64i_C0_REG_FSW{riscv::C0_REG_FSW_handler, riscv::C0_REG_FSD_printer};
const riscv::Instruction<uint64_t> instr64i_C1_ADDI{riscv::C1_ADDI_handler, riscv::C1_ADDI_printer};
const riscv::Instruction<uint64_t> instr64i_C1_JAL{riscv::C1_JAL_handler, riscv::C1_JAL_printer};
const riscv::Instruction<uint64_t> instr64i_C1_ADDIW{riscv::C1_ADDIW_handler, riscv::C1_ADDIW_printer};
const riscv::Instruction<uint64_t> instr64i_C1_LI{riscv::C1_LI_handler, riscv::C1_LI_printer};
const riscv::Instruction<uint64_t> instr64i_C1_ADDI16SP{riscv::C1_ADDI16SP_handler, riscv::C1_ADDI16SP_printer};
const riscv::Instruction<uint64_t> instr64i_C1_LUI{riscv::C1_LUI_handler, riscv::C1_ADDI16SP_printer};
const riscv::Instruction<uint64_t> instr64i_C1_ALU_OPS{riscv::C1_ALU_OPS_handler, riscv::C1_ALU_OPS_printer};
const riscv::Instruction<uint64_t> instr64i_C1_JUMP{riscv::C1_JUMP_handler, riscv::C1_JUMP_printer};
const riscv::Instruction<uint64_t> instr64i_C1_BEQZ{riscv::C1_BEQZ_handler, riscv::C1_BEQZ_printer};
const riscv::Instruction<uint64_t> instr64i_C1_BNEZ{riscv::C1_BNEZ_handler, riscv::C1_BNEZ_printer};
const riscv::Instruction<uint64_t> instr64i_C2_SLLI{riscv::C2_SLLI_handler, riscv::C2_SLLI_printer};
const riscv::Instruction<uint64_t> instr64i_C2_FLDSP{riscv::C2_FLDSP_handler, riscv::C2_SLLI_printer};
const riscv::Instruction<uint64_t> instr64i_C2_LWSP{riscv::C2_LWSP_handler, riscv::C2_SLLI_printer};
const riscv::Instruction<uint64_t> instr64i_C2_LDSP{riscv::C2_LDSP_handler, riscv::C2_SLLI_printer};
const riscv::Instruction<uint64_t> instr64i_C2_FLWSP{riscv::C2_FLWSP_handler, riscv::C2_SLLI_printer};
const riscv::Instruction<uint64_t> instr64i_C2_FSDSP{riscv::C2_FSDSP_handler, riscv::C2_FSDSP_printer};
const riscv::Instruction<uint64_t> instr64i_C2_SWSP{riscv::C2_SWSP_handler, riscv::C2_FSDSP_printer};
const riscv::Instruction<uint64_t> instr64i_C2_SDSP{riscv::C2_SDSP_handler, riscv::C2_SDSP_printer};
const riscv::Instruction<uint64_t> instr64i_C2_FSWSP{riscv::C2_FSWSP_handler, riscv::C2_FSDSP_printer};
const riscv::Instruction<uint64_t> instr64i_C2_JR{riscv::C2_JR_handler, riscv::C2_JR_printer};
const riscv::Instruction<uint64_t> instr64i_C2_JALR{riscv::C2_JALR_handler, riscv::C2_JR_printer};
const riscv::Instruction<uint64_t> instr64i_C2_MV{riscv::C2_MV_handler, riscv::C2_JR_printer};
const riscv::Instruction<uint64_t> instr64i_C2_ADD{riscv::C2_ADD_handler, riscv::C2_JR_printer};
const riscv::Instruction<uint64_t> instr64i_C2_EBREAK{riscv::C2_EBREAK_handler, riscv::C2_JR_printer};
