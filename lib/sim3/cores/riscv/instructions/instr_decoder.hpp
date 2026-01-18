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
#pragma once
#include "../../../../bts/isa/riscv/rv_instruction_list.hpp"
#include "../../../../bts/isa/riscv/rvfd.hpp"
#include "./instr_helpers.hpp"
#include "bts/isa/riscv/rv_types.hpp"
#include "bts/isa/riscv/rvi.hpp"
#include "sim3/common_macros.hpp"

#include "../../../../bts/isa/riscv/rva.hpp"
#include "../../../../bts/isa/riscv/rvv.hpp"
#include "./rva_instr.hpp"
#include "./rvc_instr.hpp"
#include "./rvfd_instr.hpp"
#include "./rvi_instr.hpp"
#include "./rvv_instr.hpp"
#include "bts/isa/riscv/rvc.hpp"
#include "sim3/systems/notraced_riscv_isa3_system/rv_common.hpp"

namespace riscv {
template <AddressType address_t>
const riscv::Instruction<address_t> &decode_one(const riscv::instruction_format instruction) {
  // -*-C++-*-
  using namespace riscv;
  if (instruction.is_long()) // RV32 IMAFD
  {
    // Quadrant 3
    switch (instruction.opcode()) {
      // RV32IM
    case RV32I_LOAD:
      if (LIKELY(instruction.Itype.rd != 0)) {
        switch (instruction.Itype.funct3) {
        case 0x0:
          if constexpr (sizeof(address_t) == 4) return instr32i_LOAD_I8;
          else return instr64i_LOAD_I8;
        case 0x1:
          if constexpr (sizeof(address_t) == 4) return instr32i_LOAD_I16;
          else return instr64i_LOAD_I16;
        case 0x2:
          if constexpr (sizeof(address_t) == 4) return instr32i_LOAD_I32;
          else return instr64i_LOAD_I32;
        case 0x3:
          if constexpr (sizeof(address_t) == 8) return instr64i_LOAD_I64;
          else return instr32i_ILLEGAL;
        case 0x4:
          if constexpr (sizeof(address_t) == 4) return instr32i_LOAD_U8;
          else return instr64i_LOAD_U8;
        case 0x5:
          if constexpr (sizeof(address_t) == 4) return instr32i_LOAD_U16;
          else return instr64i_LOAD_U16;
        case 0x6:
          if constexpr (sizeof(address_t) == 4) return instr32i_LOAD_U32;
          else return instr64i_LOAD_U32;
        case 0x7: [[fallthrough]];
        default:
          if constexpr (sizeof(address_t) == 4) return instr32i_ILLEGAL;
          else return instr64i_ILLEGAL;
        }
      }
      if constexpr (sizeof(address_t) == 4) return instr32i_LOAD_X_DUMMY;
      else return instr64i_LOAD_X_DUMMY;

    case RV32I_STORE:
      switch (instruction.Stype.funct3) {
      case 0x0:
        if (instruction.Stype.signed_imm() == 0) {
          if constexpr (sizeof(address_t) == 4) return instr32i_STORE_I8;
          else return instr64i_STORE_I8;
        }
        if constexpr (sizeof(address_t) == 4) return instr32i_STORE_I8_IMM;
        else return instr64i_STORE_I8_IMM;
      case 0x1:
        if constexpr (sizeof(address_t) == 4) return instr32i_STORE_I16_IMM;
        else return instr64i_STORE_I16_IMM;
      case 0x2:
        if constexpr (sizeof(address_t) == 4) return instr32i_STORE_I32_IMM;
        else return instr64i_STORE_I32_IMM;
      case 0x3:
        if constexpr (sizeof(address_t) == 8) return instr64i_STORE_I64_IMM;
        else return instr32i_STORE_I64_IMM;
        [[fallthrough]];
      case 0x4: [[fallthrough]];
      default:; // fallthrough
      }
      if constexpr (sizeof(address_t) == 4) return instr32i_ILLEGAL;
      else return instr64i_ILLEGAL;

    case RV32I_BRANCH:
      switch (instruction.Btype.funct3) {
      case 0x0:
        if constexpr (sizeof(address_t) == 4) return instr32i_BRANCH_EQ;
        else return instr64i_BRANCH_EQ;
      case 0x1:
        if constexpr (sizeof(address_t) == 4) return instr32i_BRANCH_NE;
        else return instr64i_BRANCH_NE;
      case 0x2: [[fallthrough]];
      case 0x3:
        if constexpr (sizeof(address_t) == 4) return instr32i_ILLEGAL;
        else return instr64i_ILLEGAL;
      case 0x4:
        if constexpr (sizeof(address_t) == 4) return instr32i_BRANCH_LT;
        else return instr64i_BRANCH_LT;
      case 0x5:
        if constexpr (sizeof(address_t) == 4) return instr32i_BRANCH_GE;
        else return instr64i_BRANCH_GE;
      case 0x6:
        if constexpr (sizeof(address_t) == 4) return instr32i_BRANCH_LTU;
        else return instr64i_BRANCH_LTU;
      case 0x7:
        if constexpr (sizeof(address_t) == 4) return instr32i_BRANCH_GEU;
        else return instr64i_BRANCH_GEU;
      }

    case RV32I_JALR:
      if constexpr (sizeof(address_t) == 4) return instr32i_JALR;
      else return instr64i_JALR;

    case RV32I_JAL:
      if (instruction.Jtype.rd != 0) {
        if constexpr (sizeof(address_t) == 4) return instr32i_JAL;
        else return instr64i_JAL;
      }
      if constexpr (sizeof(address_t) == 4) return instr32i_JMPI;
      else return instr64i_JMPI;

    case RV32I_OP_IMM:
      if (LIKELY(instruction.Itype.rd != 0)) {
        switch (instruction.Itype.funct3) {
        case 0x0: // ADDI
          if (instruction.Itype.rs1 == 0) {
            if constexpr (sizeof(address_t) == 4) return instr32i_OP_IMM_LI;
            else return instr64i_OP_IMM_LI;
          } else if (instruction.Itype.imm == 0) {
            if constexpr (sizeof(address_t) == 4) return instr32i_OP_MV;
            else return instr64i_OP_MV;
          }
          if constexpr (sizeof(address_t) == 4) return instr32i_OP_IMM_ADDI;
          else return instr64i_OP_IMM_ADDI;
        case 0x1: // SLLI
          if (instruction.Itype.high_bits() == 0x0) {
            if constexpr (sizeof(address_t) == 4) return instr32i_OP_IMM_SLLI;
            else return instr64i_OP_IMM_SLLI;
          }
          if constexpr (sizeof(address_t) == 4) return instr32i_OP_IMM;
          else return instr64i_OP_IMM;
        case 0x5: // SRLI / SRAI
          if (instruction.Itype.high_bits() == 0x0) {
            if constexpr (sizeof(address_t) == 4) return instr32i_OP_IMM_SRLI;
            else return instr64i_OP_IMM_SRLI;
          } else {
            if constexpr (sizeof(address_t) == 4) return instr32i_OP_IMM;
            else return instr64i_OP_IMM;
          }
        case 0x7: // ANDI
          if constexpr (sizeof(address_t) == 4) return instr32i_OP_IMM_ANDI;
          else return instr64i_OP_IMM_ANDI;
        default:
          if constexpr (sizeof(address_t) == 4) return instr32i_OP_IMM;
          else return instr64i_OP_IMM;
        }
      }
      if constexpr (sizeof(address_t) == 4) return instr32i_NOP;
      else return instr64i_NOP;

    case RV32I_OP:
      if (LIKELY(instruction.Rtype.rd != 0)) {
        switch (instruction.Rtype.jumptable_friendly_op()) {
        case 0x0:
          if constexpr (sizeof(address_t) == 4) return instr32i_OP_ADD;
          else return instr64i_OP_ADD;
        case 0x200:
          if constexpr (sizeof(address_t) == 4) return instr32i_OP_SUB;
          else return instr64i_OP_SUB;
        default:
          if constexpr (sizeof(address_t) == 4) return instr32i_OP;
          else return instr64i_OP;
        }
      }
      if constexpr (sizeof(address_t) == 4) return instr32i_NOP;
      else return instr64i_NOP;

    case RV32I_SYSTEM:
      if (LIKELY(instruction.Itype.funct3 == 0)) {
        if (instruction.Itype.imm == 0) {
          if constexpr (sizeof(address_t) == 4) return instr32i_SYSCALL;
          else return instr64i_SYSCALL;
        } else if (instruction.Itype.imm == 0x7FF) { // STOP
          if constexpr (sizeof(address_t) == 4) return instr32i_WFI;
          else return instr64i_WFI;
        } else if (instruction.Itype.imm == 261) {
          if constexpr (sizeof(address_t) == 4) return instr32i_WFI;
          else return instr64i_WFI;
        }
      }
      if constexpr (sizeof(address_t) == 4) return instr32i_SYSTEM;
      else return instr64i_SYSTEM;
    case RV32I_LUI:
      if (LIKELY(instruction.Utype.rd != 0)) {
        if constexpr (sizeof(address_t) == 4) return instr32i_LUI;
        else return instr64i_LUI;
      }
      if constexpr (sizeof(address_t) == 4) return instr32i_NOP;
      else return instr64i_NOP;

    case RV32I_AUIPC:
      if (LIKELY(instruction.Utype.rd != 0)) {
        if constexpr (sizeof(address_t) == 4) return instr32i_AUIPC;
        else return instr64i_AUIPC;
      }
      if constexpr (sizeof(address_t) == 4) return instr32i_NOP;
      else return instr64i_NOP;

    case RV64I_OP_IMM32:
      if (LIKELY(instruction.Itype.rd != 0)) {
        switch (instruction.Itype.funct3) {
        case 0x0: // ADDIW
          if constexpr (sizeof(address_t) == 4) return instr32i_OP_IMM32_ADDIW;
          else return instr64i_OP_IMM32_ADDIW;
        case 0x1: // SLLIW
          if (instruction.Itype.high_bits() == 0x000) {
            if constexpr (sizeof(address_t) == 4) return instr32i_OP_IMM32_SLLIW;
            else return instr64i_OP_IMM32_SLLIW;
          } else if (instruction.Itype.high_bits() == 0x080) {
            if constexpr (sizeof(address_t) == 4) return instr32i_OP_IMM32_SLLI_UW;
            else return instr64i_OP_IMM32_SLLI_UW;
          } else {
            if constexpr (sizeof(address_t) == 4) return instr32i_OP_IMM32;
            else return instr64i_OP_IMM32;
          }
        case 0x5: // SRLIW / SRAIW
          if (instruction.Itype.high_bits() == 0x000) {
            if constexpr (sizeof(address_t) == 4) return instr32i_OP_IMM32_SRLIW;
            else return instr64i_OP_IMM32_SRLIW;
          } else if (instruction.Itype.high_bits() == 0x400) {
            if constexpr (sizeof(address_t) == 4) return instr32i_OP_IMM32_SRAIW;
            else return instr64i_OP_IMM32_SRAIW;
          } else {
            if constexpr (sizeof(address_t) == 4) return instr32i_OP_IMM32;
            else return instr64i_OP_IMM32;
          }
        }
        if constexpr (sizeof(address_t) == 4) return instr32i_ILLEGAL;
        else return instr64i_ILLEGAL;
      }
      if constexpr (sizeof(address_t) == 4) return instr32i_NOP;
      else return instr64i_NOP;

    case RV64I_OP32:
      if (LIKELY(instruction.Rtype.rd != 0)) {
        switch (instruction.Rtype.jumptable_friendly_op()) {
        case 0x0: // ADDW
          if constexpr (sizeof(address_t) == 4) return instr32i_OP32_ADDW;
          else return instr64i_OP32_ADDW;
        default:
          if constexpr (sizeof(address_t) == 4) return instr32i_OP32;
          else return instr64i_OP32;
        }
      }
      if constexpr (sizeof(address_t) == 4) return instr32i_NOP;
      else return instr64i_NOP;

    case RV32I_FENCE:
      if constexpr (sizeof(address_t) == 4) return instr32i_FENCE;
      else return instr64i_FENCE;

      // RV32F & RV32D - Floating-point instructions
    case RV32F_LOAD: {
      const riscv::rv32f_instruction fi{instruction};
      switch (fi.Itype.funct3) {
      case 0x2: // FLW
        if constexpr (sizeof(address_t) == 4) return instr32i_FLW;
        else return instr64i_FLW;
      case 0x3: // FLD
        if constexpr (sizeof(address_t) == 4) return instr32i_FLD;
        else return instr64i_FLD;
      case 0x6: // VLE32
        if constexpr (sizeof(address_t) == 4) return instr32i_VLE32;
        else return instr64i_VLE32;
      default:
        if constexpr (sizeof(address_t) == 4) return instr32i_ILLEGAL;
        else return instr64i_ILLEGAL;
      }
    }
    case RV32F_STORE: {
      const rv32f_instruction fi{instruction};
      switch (fi.Itype.funct3) {
      case 0x2: // FSW
        if constexpr (sizeof(address_t) == 4) return instr32i_FSW;
        else return instr64i_FSW;
      case 0x3: // FSD
        if constexpr (sizeof(address_t) == 4) return instr32i_FSD;
        else return instr64i_FSD;
      case 0x6: // VSE32
        if constexpr (sizeof(address_t) == 4) return instr32i_VSE32;
        else return instr64i_VSE32;
      default:
        if constexpr (sizeof(address_t) == 4) return instr32i_ILLEGAL;
        else return instr64i_ILLEGAL;
      }
    }
    case RV32F_FMADD:
      if constexpr (sizeof(address_t) == 4) return instr32i_FMADD;
      else return instr64i_FMADD;
    case RV32F_FMSUB:
      if constexpr (sizeof(address_t) == 4) return instr32i_FMSUB;
      else return instr64i_FMSUB;
    case RV32F_FNMSUB:
      if constexpr (sizeof(address_t) == 4) return instr32i_FNMSUB;
      else return instr64i_FNMSUB;
    case RV32F_FNMADD:
      if constexpr (sizeof(address_t) == 4) return instr32i_FNMADD;
      else return instr64i_FNMADD;
    case RV32F_FPFUNC:
      switch (instruction.fpfunc()) {
      case 0b00000:
        if constexpr (sizeof(address_t) == 4) return instr32i_FADD;
        else return instr64i_FADD;
      case 0b00001:
        if constexpr (sizeof(address_t) == 4) return instr32i_FSUB;
        else return instr64i_FSUB;
      case 0b00010:
        if constexpr (sizeof(address_t) == 4) return instr32i_FMUL;
        else return instr64i_FMUL;
      case 0b00011:
        if constexpr (sizeof(address_t) == 4) return instr32i_FDIV;
        else return instr64i_FDIV;
      case 0b00100:
        if constexpr (sizeof(address_t) == 4) return instr32i_FSGNJ_NX;
        else return instr64i_FSGNJ_NX;
      case 0b00101:
        if constexpr (sizeof(address_t) == 4) return instr32i_FMIN_FMAX;
        else return instr64i_FMIN_FMAX;
      case 0b01011:
        if constexpr (sizeof(address_t) == 4) return instr32i_FSQRT;
        else return instr64i_FSQRT;
      case 0b10100:
        if (rv32f_instruction{instruction}.R4type.rd != 0) {
          if constexpr (sizeof(address_t) == 4) return instr32i_FEQ_FLT_FLE;
          else return instr64i_FEQ_FLT_FLE;
        }
        if constexpr (sizeof(address_t) == 4) return instr32i_NOP;
        else return instr64i_NOP;
      case 0b01000:
        if constexpr (sizeof(address_t) == 4) return instr32i_FCVT_SD_DS;
        else return instr64i_FCVT_SD_DS;
      case 0b11000:
        if (rv32f_instruction{instruction}.R4type.rd != 0) {
          if constexpr (sizeof(address_t) == 4) return instr32i_FCVT_W_SD;
          else return instr64i_FCVT_W_SD;
        }
        if constexpr (sizeof(address_t) == 4) return instr32i_NOP;
        else return instr64i_NOP;
      case 0b11010:
        if constexpr (sizeof(address_t) == 4) return instr32i_FCVT_SD_W;
        else return instr64i_FCVT_SD_W;
      case 0b11100:
        if (rv32f_instruction{instruction}.R4type.rd != 0) {
          if (rv32f_instruction{instruction}.R4type.funct3 == 0) {
            if constexpr (sizeof(address_t) == 4) return instr32i_FMV_X_W;
            else return instr64i_FMV_X_W;
          }
          if constexpr (sizeof(address_t) == 4) return instr32i_FCLASS;
          else return instr64i_FCLASS;
        }
        if constexpr (sizeof(address_t) == 4) return instr32i_NOP;
        else return instr64i_NOP;
      case 0b11110:
        if constexpr (sizeof(address_t) == 4) return instr32i_FMV_W_X;
        else return instr64i_FMV_W_X;
      }
      break;

    case RV32V_OP:
      switch (instruction.vwidth()) {
      case 0x0: // OPI.VV
        if constexpr (sizeof(address_t) == 4) return instr32i_VOPI_VV;
        else return instr64i_VOPI_VV;
      case 0x1: // OPF.VV
        if constexpr (sizeof(address_t) == 4) return instr32i_VOPF_VV;
        else return instr64i_VOPF_VV;
      case 0x2: // OPM.VV
        if constexpr (sizeof(address_t) == 4) return instr32i_VOPM_VV;
        else return instr64i_VOPM_VV;
      case 0x3: // OPI.VI
        if constexpr (sizeof(address_t) == 4) return instr32i_VOPI_VI;
        else return instr64i_VOPI_VI;
      case 0x5: // OPF.VF
        if constexpr (sizeof(address_t) == 4) return instr32i_VOPF_VF;
        else return instr64i_VOPF_VF;
      case 0x7: // Vector Configuration
        switch (instruction.vsetfunc()) {
        case 0x0: [[fallthrough]];
        case 0x1:
          if constexpr (sizeof(address_t) == 4) return instr32i_VSETVLI;
          else return instr64i_VSETVLI;
        case 0x2:
          if constexpr (sizeof(address_t) == 4) return instr32i_VSETVL;
          else return instr64i_VSETVL;
        case 0x3:
          if constexpr (sizeof(address_t) == 4) return instr32i_VSETIVLI;
          else return instr64i_VSETIVLI;
        }
      }
      break;
      // RVxA - Atomic instructions
    case RV32A_ATOMIC:
      switch (instruction.Atype.funct3) {
      case AMOSIZE_W:
        switch (instruction.Atype.funct5) {
        case 0b00010:
          if (instruction.Atype.rs2 == 0) {
            if constexpr (sizeof(address_t) == 4) return instr32i_LOAD_RESV;
            else return instr64i_LOAD_RESV;
          }
          if constexpr (sizeof(address_t) == 4) return instr32i_ILLEGAL;
          else return instr64i_ILLEGAL;
        case 0b00011:
          if constexpr (sizeof(address_t) == 4) return instr32i_STORE_COND;
          else return instr64i_STORE_COND;
        case 0b00000:
          if constexpr (sizeof(address_t) == 4) return instr32i_AMOADD_W;
          else return instr64i_AMOADD_W;
        case 0b00001:
          if constexpr (sizeof(address_t) == 4) return instr32i_AMOSWAP_W;
          else return instr64i_AMOSWAP_W;
        case 0b00100:
          if constexpr (sizeof(address_t) == 4) return instr32i_AMOXOR_W;
          else return instr64i_AMOXOR_W;
        case 0b01000:
          if constexpr (sizeof(address_t) == 4) return instr32i_AMOOR_W;
          else return instr64i_AMOOR_W;
        case 0b01100:
          if constexpr (sizeof(address_t) == 4) return instr32i_AMOAND_W;
          else return instr64i_AMOAND_W;
        case 0b10000:
          if constexpr (sizeof(address_t) == 4) return instr32i_AMOMIN_W;
          else return instr64i_AMOMIN_W;
        case 0b10100:
          if constexpr (sizeof(address_t) == 4) return instr32i_AMOMAX_W;
          else return instr64i_AMOMAX_W;
        case 0b11000:
          if constexpr (sizeof(address_t) == 4) return instr32i_AMOMINU_W;
          else return instr64i_AMOMINU_W;
        case 0b11100:
          if constexpr (sizeof(address_t) == 4) return instr32i_AMOMAXU_W;
          else return instr64i_AMOMAXU_W;
        }
        break;
      case AMOSIZE_D:
        if constexpr (sizeof(address_t) >= 8) {
          switch (instruction.Atype.funct5) {
          case 0b00010:
            if (instruction.Atype.rs2 == 0) {
              if constexpr (sizeof(address_t) == 4) return instr32i_LOAD_RESV;
              else return instr64i_LOAD_RESV;
            }
            if constexpr (sizeof(address_t) == 4) return instr32i_ILLEGAL;
            else return instr64i_ILLEGAL;
          case 0b00011:
            if constexpr (sizeof(address_t) == 4) return instr32i_STORE_COND;
            else return instr64i_STORE_COND;
          case 0b00000:
            if constexpr (sizeof(address_t) == 4) return instr32i_AMOSWAP_D;
            else return instr64i_AMOSWAP_D;
          case 0b00001:
            if constexpr (sizeof(address_t) == 4) return instr32i_AMOSWAP_D;
            else return instr64i_AMOSWAP_D;
          case 0b00100:
            if constexpr (sizeof(address_t) == 4) return instr32i_AMOXOR_D;
            else return instr64i_AMOXOR_D;
          case 0b01000:
            if constexpr (sizeof(address_t) == 4) return instr32i_AMOOR_D;
            else return instr64i_AMOOR_D;
          case 0b01100:
            if constexpr (sizeof(address_t) == 4) return instr32i_AMOAND_D;
            else return instr64i_AMOAND_D;
          case 0b10000:
            if constexpr (sizeof(address_t) == 4) return instr32i_AMOMIN_D;
            else return instr64i_AMOMIN_D;
          case 0b10100:
            if constexpr (sizeof(address_t) == 4) return instr32i_AMOMAX_D;
            else return instr64i_AMOMAX_D;
          case 0b11000:
            if constexpr (sizeof(address_t) == 4) return instr32i_AMOMINU_D;
            else return instr64i_AMOMINU_D;
          case 0b11100:
            if constexpr (sizeof(address_t) == 4) return instr32i_AMOMAXU_D;
            else return instr64i_AMOMAXU_D;
          }
          break;
        }
      }
    }
  } else if constexpr (compressed_enabled) {
    // RISC-V Compressed Extension
    const rv32c_instruction ci{instruction};
    switch (ci.opcode()) {
      // Quadrant 0
    case CI_CODE(0b000, 0b00):
      // if all bits are zero, it's an illegal instruction
      if (ci.whole != 0x0) {
        if constexpr (sizeof(address_t) == 4) return instr32i_C0_ADDI4SPN;
        else return instr64i_C0_ADDI4SPN;
      }
      if constexpr (sizeof(address_t) == 4) return instr32i_ILLEGAL;
      else return instr64i_ILLEGAL;
    case CI_CODE(0b001, 0b00): [[fallthrough]];
    case CI_CODE(0b010, 0b00): [[fallthrough]];
    case CI_CODE(0b011, 0b00):
      if (ci.CL.funct3 == 0x1) { // C.FLD
        if constexpr (sizeof(address_t) == 4) return instr32i_C0_REG_FLD;
        else return instr64i_C0_REG_FLD;
      } else if (ci.CL.funct3 == 0x2) { // C.LW
        if constexpr (sizeof(address_t) == 4) return instr32i_C0_REG_LW;
        else return instr64i_C0_REG_LW;
      } else if (ci.CL.funct3 == 0x3) {
        // C.LD / // C.FLW
        if constexpr (sizeof(address_t) == 8) return instr64i_C0_REG_LD;
        else return instr32i_C0_REG_FLW;
      }
      if constexpr (sizeof(address_t) == 4) return instr32i_ILLEGAL;
      else return instr64i_ILLEGAL;
    // RESERVED: 0b100, 0b00
    case CI_CODE(0b101, 0b00): [[fallthrough]];
    case CI_CODE(0b110, 0b00): [[fallthrough]];
    case CI_CODE(0b111, 0b00):
      switch (ci.CS.funct3) {
      case 4:
        if constexpr (sizeof(address_t) == 4) return instr32i_UNIMPLEMENTED;
        else return instr64i_UNIMPLEMENTED;
      case 5: // C.FSD
        if constexpr (sizeof(address_t) == 4) return instr32i_C0_REG_FSD;
        else return instr64i_C0_REG_FSD;
      case 6: // C.SW
        if constexpr (sizeof(address_t) == 4) return instr32i_C0_REG_SW;
        else return instr64i_C0_REG_SW;
      case 7: // C.FSW /  C.SD
        if constexpr (sizeof(address_t) == 4) return instr32i_C0_REG_FSW;
        else return instr64i_C0_REG_SD;
      }
      if constexpr (sizeof(address_t) == 4) return instr32i_ILLEGAL;
      else return instr64i_ILLEGAL;
    // Quadrant 1
    case CI_CODE(0b000, 0b01): // C.ADDI
      if (ci.CI.rd != 0) {
        if constexpr (sizeof(address_t) == 4) return instr32i_C1_ADDI;
        else return instr64i_C1_ADDI;
      }
      if constexpr (sizeof(address_t) == 4) return instr32i_NOP;
      else return instr64i_NOP;
    case CI_CODE(0b001, 0b01): // C.ADDIW / C.JAL
      if constexpr (sizeof(address_t) == 8) {
        if (ci.CI.rd != 0) return instr64i_C1_ADDIW;
        return instr64i_NOP;
      } else return instr32i_C1_JAL;

    case CI_CODE(0b010, 0b01):
      if (ci.CI.rd != 0) {
        if constexpr (sizeof(address_t) == 4) return instr32i_C1_LI;
        else return instr64i_C1_LI;
      }
      if constexpr (sizeof(address_t) == 4) return instr32i_NOP;
      else return instr64i_NOP;
    case CI_CODE(0b011, 0b01):
      if (ci.CI.rd == 2) {
        if constexpr (sizeof(address_t) == 4) return instr32i_C1_ADDI16SP;
        else return instr64i_C1_ADDI16SP;
      } else if (ci.CI.rd != 0) {
        if constexpr (sizeof(address_t) == 4) return instr32i_C1_LUI;
        else return instr64i_C1_LUI;
      }
      if constexpr (sizeof(address_t) == 4) return instr32i_ILLEGAL;
      else return instr64i_ILLEGAL;
    case CI_CODE(0b100, 0b01):
      if constexpr (sizeof(address_t) == 4) return instr32i_C1_ALU_OPS;
      else return instr64i_C1_ALU_OPS;
    case CI_CODE(0b101, 0b01):
      if constexpr (sizeof(address_t) == 4) return instr32i_C1_JUMP;
      else return instr64i_C1_JUMP;
    case CI_CODE(0b110, 0b01):
      if constexpr (sizeof(address_t) == 4) return instr32i_C1_BEQZ;
      else return instr64i_C1_BEQZ;
    case CI_CODE(0b111, 0b01):
      if constexpr (sizeof(address_t) == 4) return instr32i_C1_BNEZ;
      else return instr64i_C1_BNEZ;
    // Quadrant 2
    case CI_CODE(0b000, 0b10): [[fallthrough]];
    case CI_CODE(0b001, 0b10): [[fallthrough]];
    case CI_CODE(0b010, 0b10): [[fallthrough]];
    case CI_CODE(0b011, 0b10):
      if (ci.CI.funct3 == 0x0 && ci.CI.rd != 0) { // C.SLLI
        if constexpr (sizeof(address_t) == 4) return instr32i_C2_SLLI;
        else return instr64i_C2_SLLI;
      } else if (ci.CI2.funct3 == 0x1) { // C.FLDSP
        if constexpr (sizeof(address_t) == 4) return instr32i_C2_FLDSP;
        else return instr64i_C2_FLDSP;
      } else if (ci.CI2.funct3 == 0x2 && ci.CI2.rd != 0) { // C.LWSP
        if constexpr (sizeof(address_t) == 4) return instr32i_C2_LWSP;
        else return instr64i_C2_LWSP;
      } else if (ci.CI2.funct3 == 0x3) {
        // // C.LDSP / C.FLWSP
        if constexpr (sizeof(address_t) == 8) {
          if (ci.CI2.rd != 0) return instr64i_C2_LDSP;
        } else return instr32i_C2_FLWSP;
      } else if (ci.CI.rd == 0) { // C.HINT
        if constexpr (sizeof(address_t) == 4) return instr32i_NOP;
        else return instr64i_NOP;
      }
      if constexpr (sizeof(address_t) == 4) return instr32i_UNIMPLEMENTED;
      else return instr64i_UNIMPLEMENTED;
    case CI_CODE(0b100, 0b10): {
      const bool topbit = ci.whole & (1 << 12);
      if (!topbit && ci.CR.rd != 0 && ci.CR.rs2 == 0) { // JR rd
        if constexpr (sizeof(address_t) == 4) return instr32i_C2_JR;
        else return instr64i_C2_JR;
      } else if (topbit && ci.CR.rd != 0 && ci.CR.rs2 == 0) { // JALR ra, rd+0
        if constexpr (sizeof(address_t) == 4) return instr32i_C2_JALR;
        else return instr64i_C2_JALR;
      } else if (!topbit && ci.CR.rd != 0 && ci.CR.rs2 != 0) { // MV rd, rs2
        if constexpr (sizeof(address_t) == 4) return instr32i_C2_MV;
        else return instr64i_C2_MV;
      } else if (ci.CR.rd != 0) { // ADD rd, rd + rs2
        if constexpr (sizeof(address_t) == 4) return instr32i_C2_ADD;
        else return instr64i_C2_ADD;
      } else if (topbit && ci.CR.rd == 0 && ci.CR.rs2 == 0) { // EBREAK
        if constexpr (sizeof(address_t) == 4) return instr32i_C2_EBREAK;
        else return instr64i_C2_EBREAK;
      }
      if constexpr (sizeof(address_t) == 4) return instr32i_UNIMPLEMENTED;
      else return instr64i_UNIMPLEMENTED;
    }
    case CI_CODE(0b101, 0b10): [[fallthrough]];
    case CI_CODE(0b110, 0b10): [[fallthrough]];
    case CI_CODE(0b111, 0b10):
      if (ci.CSS.funct3 == 5) { // FSDSP
        if constexpr (sizeof(address_t) == 4) return instr32i_C2_FSDSP;
        else return instr64i_C2_FSDSP;
      } else if (ci.CSS.funct3 == 6) { // SWSP
        if constexpr (sizeof(address_t) == 4) return instr32i_C2_SWSP;
        else return instr64i_C2_SWSP;
      } else if (ci.CSS.funct3 == 7) {
        // SDSP / FSWP
        if constexpr (sizeof(address_t) == 8) return instr64i_C2_SDSP;
        else return instr32i_C2_FSWSP;
      }
      if constexpr (sizeof(address_t) == 4) return instr32i_UNIMPLEMENTED;
      else return instr64i_UNIMPLEMENTED;
    }
  }
  // all zeroes: illegal instruction
  if (instruction.whole == 0x0) {
    if constexpr (sizeof(address_t) == 4) return instr32i_ILLEGAL;
    else return instr64i_ILLEGAL;
  }

  if constexpr (sizeof(address_t) == 4) return instr32i_UNIMPLEMENTED;
  else return instr64i_UNIMPLEMENTED;
}
} // namespace riscv
