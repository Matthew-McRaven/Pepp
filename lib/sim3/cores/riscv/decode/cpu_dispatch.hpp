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
#include "../instruction_counter.hpp"
#include "decoder_cache_impl.hpp"
#include "enums/isa/rva.hpp"
#include "enums/isa/rvc.hpp"
#include "enums/isa/rvfd.hpp"
#include "enums/isa/rvi.hpp"
#include "enums/isa/rvv.hpp"
#include "sim3/systems/notraced_riscv_isa3_system.hpp"
#include "threaded_bytecodes.hpp"

#define DISPATCH_MODE_SWITCH_BASED
#define DISPATCH_FUNC simulate_bytecode

/**
 * This file is included by threaded_dispatch.cpp and bytecode_dispatch.cpp
 * It implements the logic for switch-based and threaded dispatch.
 *
 * All dispatch modes share bytecode_impl.cpp
 **/
namespace riscv {
static constexpr bool VERBOSE_JUMPS = riscv::verbose_branches_enabled;
#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
static constexpr bool FUZZING = true;
#else
static constexpr bool FUZZING = false;
#endif

#define VIEW_INSTR() auto instr = *(rv32i_instruction *)&decoder->instr;
#define VIEW_INSTR_AS(name, x) auto &&name = *(x *)&decoder->instr;
#define NEXT_INSTR()                                                                                                   \
  if constexpr (compressed_enabled) decoder += 2;                                                                      \
  else decoder += 1;                                                                                                   \
  continue;                                                                                                            \
  ;
#define NEXT_C_INSTR()                                                                                                 \
  decoder += 1;                                                                                                        \
  continue;

#define NEXT_BLOCK(len, OF)                                                                                            \
  pc += len;                                                                                                           \
  decoder += len >> DecoderCache<address_t>::SHIFT;                                                                    \
  if constexpr (FUZZING) /* Give OOB-aid to ASAN */                                                                    \
    decoder = &exec_decoder[pc >> DecoderCache<address_t>::SHIFT];                                                     \
  if constexpr (OF) {                                                                                                  \
    if (UNLIKELY(counter.overflowed())) goto check_jump;                                                               \
  }                                                                                                                    \
  pc += decoder->block_bytes();                                                                                        \
  counter.increment_counter(decoder->instruction_count());                                                             \
  continue;                                                                                                            \
  ;

#define SAFE_INSTR_NEXT(len)                                                                                           \
  pc += len;                                                                                                           \
  decoder += len >> DecoderCache<address_t>::SHIFT;                                                                    \
  counter.increment_counter(1);

#define NEXT_SEGMENT()                                                                                                 \
  decoder = &exec_decoder[pc >> DecoderCache<address_t>::SHIFT];                                                       \
  pc += decoder->block_bytes();                                                                                        \
  counter.increment_counter(decoder->instruction_count());                                                             \
  continue;

#define PERFORM_BRANCH()                                                                                               \
  if constexpr (VERBOSE_JUMPS)                                                                                         \
    fprintf(stderr, "Branch 0x%lX >= 0x%lX (decoder=%p)\n", long(pc), long(pc + fi.signed_imm()), decoder);            \
  if (LIKELY(!counter.overflowed())) {                                                                                 \
    NEXT_BLOCK(fi.signed_imm(), false);                                                                                \
  }                                                                                                                    \
  pc += fi.signed_imm();                                                                                               \
  goto check_jump;

#define PERFORM_FORWARD_BRANCH()                                                                                       \
  if constexpr (VERBOSE_JUMPS) fprintf(stderr, "Fw.Branch 0x%lX >= 0x%lX\n", long(pc), long(pc + fi.signed_imm()));    \
  NEXT_BLOCK(fi.signed_imm(), false);

#define OVERFLOW_CHECKED_JUMP() goto check_jump

template <AddressType address_t>
PEPP_HOT_PATH()
bool CPU<address_t>::simulate(address_t pc, uint64_t inscounter, uint64_t maxcounter) {
  static constexpr auto W = sizeof(address_t);
  static constexpr uint32_t XLEN = W * 8;
  using addr_t = address_t;
  using saddr_t = ToSignedAddress<addr_t>;

  DecodedExecuteSegment<address_t> *exec = this->m_exec;
  DecoderData<address_t> *exec_decoder = exec->decoder_cache();
  DecoderData<address_t> *decoder;

  address_t current_begin = exec->exec_begin();
  address_t current_end = exec->exec_end();

  InstrCounter counter{inscounter, maxcounter};

  // We need an execute segment matching current PC
  if (UNLIKELY(!(pc >= current_begin && pc < current_end))) goto new_execute_segment;

continue_segment:
  decoder = &exec_decoder[pc >> DecoderCache<address_t>::SHIFT];

  pc += decoder->block_bytes();
  counter.increment_counter(decoder->instruction_count());

  while (true) {
    switch (decoder->get_bytecode()) {
#define INSTRUCTION(bc, lbl) case bc:
#define REG(x) registers().get()[x]

      /** Instruction handlers **/

#ifdef RISCV_EXT_COMPRESSED
    case RV32C_BC_ADDI: {
      VIEW_INSTR_AS(fi, FasterItype);
      REG(fi.get_rs1()) = REG(fi.get_rs2()) + fi.signed_imm();
      NEXT_C_INSTR();
    }
    case RV32C_BC_MV: {
      VIEW_INSTR_AS(fi, FasterMove);
      REG(fi.get_rd()) = REG(fi.get_rs1());
      NEXT_C_INSTR();
    }
    case RV32C_BC_SLLI: {
      VIEW_INSTR_AS(fi, FasterItype);
      REG(fi.get_rs1()) <<= fi.imm;
      NEXT_C_INSTR();
    }
#endif
    case RV32I_BC_ADDI: {
      VIEW_INSTR_AS(fi, FasterItype);
      REG(fi.get_rs1()) = REG(fi.get_rs2()) + fi.signed_imm();
      NEXT_INSTR();
    }
    case RV32I_BC_LI: {
      VIEW_INSTR_AS(fi, FasterImmediate);
      REG(fi.get_rd()) = fi.signed_imm();
      NEXT_INSTR();
    }
    case RV32I_BC_MV: {
      VIEW_INSTR_AS(fi, FasterMove);
      REG(fi.get_rd()) = REG(fi.get_rs1());
      NEXT_INSTR();
    }
#ifdef RISCV_64I
    case RV64I_BC_ADDIW: {
      if constexpr (W >= 8) {
        VIEW_INSTR_AS(fi, FasterItype);
        REG(fi.get_rs1()) = (int32_t)((uint32_t)REG(fi.get_rs2()) + fi.signed_imm());
        NEXT_INSTR();
      } else PEPP_UNREACHABLE();
    }
#endif // RISCV_64I

    case RV32I_BC_FAST_JAL: {
      if constexpr (VERBOSE_JUMPS) {
        VIEW_INSTR();
        fprintf(stderr, "FAST_JAL PC 0x%lX => 0x%lX\n", long(pc), long(pc + int32_t(instr.whole)));
      }
      NEXT_BLOCK(int32_t((*decoder).instr), true);
    }
    case RV32I_BC_FAST_CALL: {
      if constexpr (VERBOSE_JUMPS) {
        VIEW_INSTR();
        fprintf(stderr, "FAST_CALL PC 0x%lX => 0x%lX\n", long(pc), long(pc + int32_t(instr.whole)));
      }
      REG(REG_RA) = pc + 4;
      NEXT_BLOCK(int32_t((*decoder).instr), true);
    }

    case RV32I_BC_SLLI: {
      VIEW_INSTR_AS(fi, FasterItype);
      // SLLI: Logical left-shift 5/6/7-bit immediate
      REG(fi.get_rs1()) = REG(fi.get_rs2()) << fi.unsigned_imm();
      NEXT_INSTR();
    }
    case RV32I_BC_SLTI: {
      VIEW_INSTR_AS(fi, FasterItype);
      // SLTI: Set less than immediate
      REG(fi.get_rs1()) = (saddr_t(REG(fi.get_rs2())) < fi.signed_imm());
      NEXT_INSTR();
    }
    case RV32I_BC_SLTIU: {
      VIEW_INSTR_AS(fi, FasterItype);
      // SLTIU: Sign-extend, then treat as unsigned
      REG(fi.get_rs1()) = (REG(fi.get_rs2()) < addr_t(fi.signed_imm()));
      NEXT_INSTR();
    }
    case RV32I_BC_XORI: {
      VIEW_INSTR_AS(fi, FasterItype);
      // XORI
      REG(fi.get_rs1()) = REG(fi.get_rs2()) ^ fi.signed_imm();
      NEXT_INSTR();
    }
    case RV32I_BC_SRLI: {
      VIEW_INSTR_AS(fi, FasterItype);
      // SRLI: Shift-right logical 5/6/7-bit immediate
      REG(fi.get_rs1()) = REG(fi.get_rs2()) >> fi.unsigned_imm();
      NEXT_INSTR();
    }
    case RV32I_BC_SRAI: {
      VIEW_INSTR_AS(fi, FasterItype);
      // SRAI: Shift-right arithmetical (preserve the sign bit)
      REG(fi.get_rs1()) = saddr_t(REG(fi.get_rs2())) >> fi.unsigned_imm();
      NEXT_INSTR();
    }
    case RV32I_BC_ORI: {
      VIEW_INSTR_AS(fi, FasterItype);
      // ORI: Or sign-extended 12-bit immediate
      REG(fi.get_rs1()) = REG(fi.get_rs2()) | fi.signed_imm();
      NEXT_INSTR();
    }
    case RV32I_BC_ANDI: {
      VIEW_INSTR_AS(fi, FasterItype);
      // ANDI: And sign-extended 12-bit immediate
      REG(fi.get_rs1()) = REG(fi.get_rs2()) & fi.signed_imm();
      NEXT_INSTR();
    }

#ifdef RISCV_EXT_COMPRESSED
    case RV32C_BC_BNEZ: {
      VIEW_INSTR_AS(fi, FasterItype);
      if (REG(fi.get_rs1()) != 0) {
        PERFORM_BRANCH();
      }
      NEXT_BLOCK(2, false);
    }
    case RV32C_BC_BEQZ: {
      VIEW_INSTR_AS(fi, FasterItype);
      if (REG(fi.get_rs1()) == 0) {
        PERFORM_BRANCH();
      }
      NEXT_BLOCK(2, false);
    }
    case RV32C_BC_JMP: {
      VIEW_INSTR_AS(fi, FasterItype);
      PERFORM_BRANCH();
    }
    case RV32C_BC_JAL_ADDIW: {
      if constexpr (W >= 8) { // C.ADDIW
        VIEW_INSTR_AS(fi, FasterItype);
        REG(fi.get_rs1()) = (int32_t)((uint32_t)REG(fi.get_rs1()) + fi.signed_imm());
        NEXT_C_INSTR();
      } else { // C.JAL
        VIEW_INSTR_AS(fi, FasterItype);
        REG(REG_RA) = pc + 2;
        PERFORM_BRANCH();
      }
    }
    case RV32C_BC_JR: {
      VIEW_INSTR();
      if constexpr (VERBOSE_JUMPS) {
        fprintf(stderr, "C.JR from 0x%lX to 0x%lX\n", long(pc), long(REG(instr.whole)));
      }
      pc = REG(instr.whole) & ~addr_t(1);
      OVERFLOW_CHECKED_JUMP();
    }
    case RV32C_BC_JALR: {
      VIEW_INSTR();
      if constexpr (VERBOSE_JUMPS) {
        fprintf(stderr, "C.JALR from 0x%lX to 0x%lX\n", long(pc), long(REG(instr.whole)));
      }
      REG(REG_RA) = pc + 2;
      pc = REG(instr.whole) & ~addr_t(1);
      OVERFLOW_CHECKED_JUMP();
    }
#endif // RISCV_EXT_COMPRESSED

    case RV32I_BC_JAL: {
      VIEW_INSTR_AS(fi, FasterJtype);
      if constexpr (VERBOSE_JUMPS) {
        printf("JAL PC 0x%lX => 0x%lX\n", (long)pc, (long)pc + fi.signed_imm());
      }
      REG(fi.rd) = pc + 4;
      NEXT_BLOCK(fi.signed_imm(), true);
    }

    case RV32I_BC_BEQ: {
      VIEW_INSTR_AS(fi, FasterItype);
      if (REG(fi.get_rs1()) == REG(fi.get_rs2())) {
        PERFORM_BRANCH();
      }
      NEXT_BLOCK(4, false);
    }
    case RV32I_BC_BNE: {
      VIEW_INSTR_AS(fi, FasterItype);
      if (REG(fi.get_rs1()) != REG(fi.get_rs2())) {
        PERFORM_BRANCH();
      }
      NEXT_BLOCK(4, false);
    }
    case RV32I_BC_BEQ_FW: {
      VIEW_INSTR_AS(fi, FasterItype);
      if (REG(fi.get_rs1()) == REG(fi.get_rs2())) {
        PERFORM_FORWARD_BRANCH();
      }
      NEXT_BLOCK(4, false);
    }
    case RV32I_BC_BNE_FW: {
      VIEW_INSTR_AS(fi, FasterItype);
      if (REG(fi.get_rs1()) != REG(fi.get_rs2())) {
        PERFORM_FORWARD_BRANCH();
      }
      NEXT_BLOCK(4, false);
    }
    case RV32I_BC_BLT: {
      VIEW_INSTR_AS(fi, FasterItype);
      if ((saddr_t)REG(fi.get_rs1()) < (saddr_t)REG(fi.get_rs2())) {
        PERFORM_BRANCH();
      }
      NEXT_BLOCK(4, false);
    }
    case RV32I_BC_BGE: {
      VIEW_INSTR_AS(fi, FasterItype);
      if ((saddr_t)REG(fi.get_rs1()) >= (saddr_t)REG(fi.get_rs2())) {
        PERFORM_BRANCH();
      }
      NEXT_BLOCK(4, false);
    }
    case RV32I_BC_BLTU: {
      VIEW_INSTR_AS(fi, FasterItype);
      if (REG(fi.get_rs1()) < REG(fi.get_rs2())) {
        PERFORM_BRANCH();
      }
      NEXT_BLOCK(4, false);
    }
    case RV32I_BC_BGEU: {
      VIEW_INSTR_AS(fi, FasterItype);
      if (REG(fi.get_rs1()) >= REG(fi.get_rs2())) {
        PERFORM_BRANCH();
      }
      NEXT_BLOCK(4, false);
    }

    case RV32I_BC_LDW: {
      VIEW_INSTR_AS(fi, FasterItype);
      const auto addr = REG(fi.get_rs2()) + fi.signed_imm();
      REG(fi.get_rs1()) = (int32_t)(*this).memory().template read<uint32_t>(addr);
      NEXT_INSTR();
    }
    case RV32I_BC_STW: {
      VIEW_INSTR_AS(fi, FasterItype);
      const auto addr = REG(fi.get_rs1()) + fi.signed_imm();
      (*this).memory().template write<uint32_t>(addr, REG(fi.get_rs2()));
      NEXT_INSTR();
    }
#ifdef RISCV_64I
    case RV32I_BC_LDD: {
      if constexpr (W >= 8) {
        VIEW_INSTR_AS(fi, FasterItype);
        const auto addr = REG(fi.get_rs2()) + fi.signed_imm();
        REG(fi.get_rs1()) = (int64_t)(*this).memory().template read<uint64_t>(addr);
        NEXT_INSTR();
      } else PEPP_UNREACHABLE();
    }
    case RV32I_BC_STD: {
      if constexpr (W >= 8) {
        VIEW_INSTR_AS(fi, FasterItype);
        const auto addr = REG(fi.get_rs1()) + fi.signed_imm();
        (*this).memory().template write<uint64_t>(addr, REG(fi.get_rs2()));
        NEXT_INSTR();
      } else PEPP_UNREACHABLE();
    }
#endif // RISCV_64I

    case RV32F_BC_FLW: {
      VIEW_INSTR_AS(fi, FasterItype);
      auto addr = REG(fi.rs2) + fi.signed_imm();
      auto &dst = registers().getfl(fi.rs1);
      dst.load_u32((*this).memory().template read<uint32_t>(addr));
      NEXT_INSTR();
    }
    case RV32F_BC_FLD: {
      VIEW_INSTR_AS(fi, FasterItype);
      auto addr = REG(fi.rs2) + fi.signed_imm();
      auto &dst = registers().getfl(fi.rs1);
      dst.load_u64((*this).memory().template read<uint64_t>(addr));
      NEXT_INSTR();
    }

#ifdef RISCV_EXT_COMPRESSED
    case RV32C_BC_LDD: {
      if constexpr (W >= 8) {
        VIEW_INSTR_AS(fi, FasterItype);
        const auto addr = REG(fi.get_rs2()) + fi.signed_imm();
        REG(fi.get_rs1()) = (int64_t)(*this).memory().template read<uint64_t>(addr);
        NEXT_C_INSTR();
      } else PEPP_UNREACHABLE();
    }
    case RV32C_BC_STD: {
      if constexpr (W >= 8) {
        VIEW_INSTR_AS(fi, FasterItype);
        const auto addr = REG(fi.get_rs1()) + fi.signed_imm();
        (*this).memory().template write<uint64_t>(addr, REG(fi.get_rs2()));
        NEXT_C_INSTR();
      } else PEPP_UNREACHABLE();
    }
    case RV32C_BC_LDW: {
      VIEW_INSTR_AS(fi, FasterItype);
      const auto addr = REG(fi.get_rs2()) + fi.signed_imm();
      REG(fi.get_rs1()) = (int32_t)(*this).memory().template read<uint32_t>(addr);
      NEXT_C_INSTR();
    }
    case RV32C_BC_STW: {
      VIEW_INSTR_AS(fi, FasterItype);
      const auto addr = REG(fi.get_rs1()) + fi.signed_imm();
      (*this).memory().template write<uint32_t>(addr, REG(fi.get_rs2()));
      NEXT_C_INSTR();
    }
#endif // RISCV_EXT_COMPRESSED

    case RV32I_BC_AUIPC: {
      VIEW_INSTR_AS(fi, FasterJtype);
      // AUIPC using re-constructed PC
      // REG(instr.Utype.rd) = (pc - (*decoder).block_bytes()) + instr.Utype.upper_imm();
      REG(fi.rd) = (pc - (*decoder).block_bytes()) + fi.upper_imm();
      NEXT_INSTR();
    }
    case RV32I_BC_LUI: {
      VIEW_INSTR_AS(fi, FasterJtype);
      REG(fi.rd) = fi.upper_imm();
      NEXT_INSTR();
    }

    case RV32I_BC_OP_ADD: {
      VIEW_INSTR_AS(fi, FasterOpType);
      auto &dst = REG(fi.get_rd());
      const auto src1 = REG(fi.get_rs1());
      const auto src2 = REG(fi.get_rs2());
      dst = src1 + src2;
      NEXT_INSTR();
    }
    case RV32I_BC_OP_SUB: {
      VIEW_INSTR_AS(fi, FasterOpType);
      auto &dst = REG(fi.get_rd());
      const auto src1 = REG(fi.get_rs1());
      const auto src2 = REG(fi.get_rs2());
      dst = src1 - src2;
      NEXT_INSTR();
    }

    case RV32I_BC_OP_SLL: {
      VIEW_INSTR_AS(fi, FasterOpType);
      auto &dst = REG(fi.get_rd());
      const auto src1 = REG(fi.get_rs1());
      const auto src2 = REG(fi.get_rs2());
      dst = src1 << (src2 & (XLEN - 1));
      NEXT_INSTR();
    }
    case RV32I_BC_OP_SLT: {
      VIEW_INSTR_AS(fi, FasterOpType);
      auto &dst = REG(fi.get_rd());
      const auto src1 = REG(fi.get_rs1());
      const auto src2 = REG(fi.get_rs2());
      dst = (saddr_t(src1) < saddr_t(src2));
      NEXT_INSTR();
    }
    case RV32I_BC_OP_SLTU: {
      VIEW_INSTR_AS(fi, FasterOpType);
      auto &dst = REG(fi.get_rd());
      const auto src1 = REG(fi.get_rs1());
      const auto src2 = REG(fi.get_rs2());
      dst = (src1 < src2);
      NEXT_INSTR();
    }
    case RV32I_BC_OP_XOR: {
      VIEW_INSTR_AS(fi, FasterOpType);
      auto &dst = REG(fi.get_rd());
      const auto src1 = REG(fi.get_rs1());
      const auto src2 = REG(fi.get_rs2());
      dst = src1 ^ src2;
      NEXT_INSTR();
    }
    case RV32I_BC_OP_SRL: {
      VIEW_INSTR_AS(fi, FasterOpType);
      auto &dst = REG(fi.get_rd());
      const auto src1 = REG(fi.get_rs1());
      const auto src2 = REG(fi.get_rs2());
      dst = src1 >> (src2 & (XLEN - 1));
      NEXT_INSTR();
    }
    case RV32I_BC_OP_OR: {
      VIEW_INSTR_AS(fi, FasterOpType);
      auto &dst = REG(fi.get_rd());
      const auto src1 = REG(fi.get_rs1());
      const auto src2 = REG(fi.get_rs2());
      dst = src1 | src2;
      NEXT_INSTR();
    }
    case RV32I_BC_OP_AND: {
      VIEW_INSTR_AS(fi, FasterOpType);
      auto &dst = REG(fi.get_rd());
      const auto src1 = REG(fi.get_rs1());
      const auto src2 = REG(fi.get_rs2());
      dst = src1 & src2;
      NEXT_INSTR();
    }
    case RV32I_BC_OP_MUL: {
      VIEW_INSTR_AS(fi, FasterOpType);
      auto &dst = REG(fi.get_rd());
      const auto src1 = REG(fi.get_rs1());
      const auto src2 = REG(fi.get_rs2());
      dst = saddr_t(src1) * saddr_t(src2);
      NEXT_INSTR();
    }
    case RV32I_BC_OP_SH1ADD: {
      VIEW_INSTR_AS(fi, FasterOpType);
      auto &dst = REG(fi.get_rd());
      const auto src1 = REG(fi.get_rs1());
      const auto src2 = REG(fi.get_rs2());
      dst = src2 + (src1 << 1);
      NEXT_INSTR();
    }
    case RV32I_BC_OP_SH2ADD: {
      VIEW_INSTR_AS(fi, FasterOpType);
      auto &dst = REG(fi.get_rd());
      const auto src1 = REG(fi.get_rs1());
      const auto src2 = REG(fi.get_rs2());
      dst = src2 + (src1 << 2);
      NEXT_INSTR();
    }
    case RV32I_BC_OP_SH3ADD: {
      VIEW_INSTR_AS(fi, FasterOpType);
      auto &dst = REG(fi.get_rd());
      const auto src1 = REG(fi.get_rs1());
      const auto src2 = REG(fi.get_rs2());
      dst = src2 + (src1 << 3);
      NEXT_INSTR();
    }
    case RV32I_BC_OP_SRA: {
      VIEW_INSTR_AS(fi, FasterOpType);
      auto &dst = REG(fi.get_rd());
      const auto src1 = REG(fi.get_rs1());
      const auto src2 = REG(fi.get_rs2());
      dst = saddr_t(src1) >> (src2 & (XLEN - 1));
      NEXT_INSTR();
    }
    case RV32I_BC_OP_ZEXT_H: {
      VIEW_INSTR_AS(fi, FasterOpType);
      auto &dst = REG(fi.get_rd());
      const auto src1 = REG(fi.get_rs1());
      const auto src2 = REG(fi.get_rs2());
      dst = uint16_t(src1);
      (void)src2;
      NEXT_INSTR();
    }

#ifdef RISCV_64I
    case RV64I_BC_OP_ADDW: {
      if constexpr (W >= 8) {
        VIEW_INSTR_AS(fi, FasterOpType);
        auto &dst = REG(fi.get_rd());
        const auto src1 = REG(fi.get_rs1());
        const auto src2 = REG(fi.get_rs2());
        dst = int32_t(uint32_t(src1) + uint32_t(src2));
        NEXT_INSTR();
      } else PEPP_UNREACHABLE();
    }
    case RV64I_BC_OP_SUBW: {
      if constexpr (W >= 8) {
        VIEW_INSTR_AS(fi, FasterOpType);
        auto &dst = REG(fi.get_rd());
        const auto src1 = REG(fi.get_rs1());
        const auto src2 = REG(fi.get_rs2());
        dst = int32_t(uint32_t(src1) - uint32_t(src2));
        NEXT_INSTR();
      } else PEPP_UNREACHABLE();
    }
    case RV64I_BC_OP_MULW: {
      if constexpr (W >= 8) {
        VIEW_INSTR_AS(fi, FasterOpType);
        auto &dst = REG(fi.get_rd());
        const auto src1 = REG(fi.get_rs1());
        const auto src2 = REG(fi.get_rs2());
        dst = int32_t(int32_t(src1) * int32_t(src2));
        NEXT_INSTR();
      } else PEPP_UNREACHABLE();
    }
    case RV64I_BC_OP_ADD_UW: {
      if constexpr (W >= 8) {
        VIEW_INSTR_AS(fi, FasterOpType);
        auto &dst = REG(fi.get_rd());
        const auto src1 = REG(fi.get_rs1());
        const auto src2 = REG(fi.get_rs2());
        dst = uint32_t(src1) + src2;
        NEXT_INSTR();
      } else PEPP_UNREACHABLE();
    }
    case RV64I_BC_SLLIW: {
      if constexpr (W >= 8) {
        VIEW_INSTR_AS(fi, FasterItype);
        REG(fi.get_rs1()) = (int32_t)((uint32_t)REG(fi.get_rs2()) << fi.imm);
        NEXT_INSTR();
      } else PEPP_UNREACHABLE();
    }
    case RV64I_BC_SRLIW: {
      if constexpr (W >= 8) {
        VIEW_INSTR_AS(fi, FasterItype);
        REG(fi.get_rs1()) = (int32_t)((uint32_t)REG(fi.get_rs2()) >> fi.imm);
        NEXT_INSTR();
      } else PEPP_UNREACHABLE();
    }
#endif // RISCV_64I

    case RV32I_BC_SEXT_B: {
      VIEW_INSTR_AS(fi, FasterItype);
      REG(fi.get_rs1()) = saddr_t(int8_t(REG(fi.get_rs2())));
      NEXT_INSTR();
    }
    case RV32I_BC_SEXT_H: {
      VIEW_INSTR_AS(fi, FasterItype);
      REG(fi.get_rs1()) = saddr_t(int16_t(REG(fi.get_rs2())));
      NEXT_INSTR();
    }
    case RV32I_BC_BSETI: {
      VIEW_INSTR_AS(fi, FasterItype);
      // BSETI: Bit-set immediate
      REG(fi.get_rs1()) = REG(fi.get_rs2()) | (addr_t(1) << fi.unsigned_imm());
      NEXT_INSTR();
    }
    case RV32I_BC_BEXTI: {
      VIEW_INSTR_AS(fi, FasterItype);
      // BEXTI: Single-bit Extract
      REG(fi.get_rs1()) = (REG(fi.get_rs2()) >> fi.unsigned_imm()) & 1;
      NEXT_INSTR();
    }

    case RV32F_BC_FSW: {
      VIEW_INSTR_AS(fi, FasterItype);
      const auto &src = registers().getfl(fi.rs2);
      auto addr = REG(fi.rs1) + fi.signed_imm();
      (*this).memory().template write<uint32_t>(addr, src.i32[0]);
      NEXT_INSTR();
    }
    case RV32F_BC_FSD: {
      VIEW_INSTR_AS(fi, FasterItype);
      const auto &src = registers().getfl(fi.rs2);
      auto addr = REG(fi.rs1) + fi.signed_imm();
      (*this).memory().template write<uint64_t>(addr, src.i64);
      NEXT_INSTR();
    }
    case RV32F_BC_FADD: {
      VIEW_INSTR_AS(fi, FasterFloatType);
      auto &dst = registers().getfl(fi.get_rd());
      const auto &rs1 = registers().getfl(fi.get_rs1());
      const auto &rs2 = registers().getfl(fi.get_rs2());
      if (fi.func == 0x0) { // float32
        dst.set_float(rs1.f32[0] + rs2.f32[0]);
      } else { // float64
        dst.f64 = rs1.f64 + rs2.f64;
      }
      NEXT_INSTR();
    }
    case RV32F_BC_FSUB: {
      VIEW_INSTR_AS(fi, FasterFloatType);
      auto &dst = registers().getfl(fi.get_rd());
      const auto &rs1 = registers().getfl(fi.get_rs1());
      const auto &rs2 = registers().getfl(fi.get_rs2());
      if (fi.func == 0x0) dst.set_float(rs1.f32[0] - rs2.f32[0]); // float32
      else dst.f64 = rs1.f64 - rs2.f64;                           // float64
      NEXT_INSTR();
    }
    case RV32F_BC_FMUL: {
      VIEW_INSTR_AS(fi, FasterFloatType);
      auto &dst = registers().getfl(fi.get_rd());
      const auto &rs1 = registers().getfl(fi.get_rs1());
      const auto &rs2 = registers().getfl(fi.get_rs2());
      if (fi.func == 0x0) dst.set_float(rs1.f32[0] * rs2.f32[0]); // float32
      else dst.f64 = rs1.f64 * rs2.f64;                           // float64
      NEXT_INSTR();
    }
    case RV32F_BC_FDIV: {
      VIEW_INSTR_AS(fi, FasterFloatType);
      auto &dst = registers().getfl(fi.get_rd());
      const auto &rs1 = registers().getfl(fi.get_rs1());
      const auto &rs2 = registers().getfl(fi.get_rs2());
      if (fi.func == 0x0) dst.set_float(rs1.f32[0] / rs2.f32[0]); // float32
      else dst.f64 = rs1.f64 / rs2.f64;                           // float64
      NEXT_INSTR();
    }
    case RV32F_BC_FMADD: {
      VIEW_INSTR_AS(fi, rv32f_instruction);
      auto &dst = registers().getfl(fi.R4type.rd);
      auto &rs1 = registers().getfl(fi.R4type.rs1);
      auto &rs2 = registers().getfl(fi.R4type.rs2);
      auto &rs3 = registers().getfl(fi.R4type.rs3);
      if (fi.R4type.funct2 == 0x0) dst.set_float(rs1.f32[0] * rs2.f32[0] + rs3.f32[0]); // float32
      else if (fi.R4type.funct2 == 0x1) dst.f64 = rs1.f64 * rs2.f64 + rs3.f64;          // float64
      NEXT_INSTR();
    }

    case RV32I_BC_JALR: {
      VIEW_INSTR_AS(fi, FasterItype);
      // jump to register + immediate
      // NOTE: if rs1 == rd, avoid clobber by storing address first
      const auto address = REG(fi.rs2) + fi.signed_imm();
      // Link *next* instruction (rd = PC + 4)
      if (fi.rs1 != 0) REG(fi.rs1) = pc + 4;

      if constexpr (VERBOSE_JUMPS) {
        fprintf(stderr, "JALR x%d + %d => rd=%d   PC 0x%lX => 0x%lX\n", fi.rs2, fi.signed_imm(), fi.rs1, long(pc),
                long(address));
      }
      static constexpr addr_t ALIGN_MASK = (compressed_enabled) ? 0x1 : 0x3;
      pc = address & ~ALIGN_MASK;
      OVERFLOW_CHECKED_JUMP();
    }

#ifdef RISCV_64I
    case RV64I_BC_SRAIW: {
      if constexpr (W >= 8) {
        VIEW_INSTR_AS(fi, FasterItype);
        // dst = (int32_t)src >> instr.Itype.shift_imm();
        REG(fi.get_rs1()) = (int32_t)REG(fi.get_rs2()) >> fi.imm;
        NEXT_INSTR();
      } else PEPP_UNREACHABLE();
    }
    case RV64I_BC_OP_SH1ADD_UW: {
      if constexpr (W >= 8) {
        VIEW_INSTR_AS(fi, FasterOpType);
        auto &dst = REG(fi.get_rd());
        const auto src1 = REG(fi.get_rs1());
        const auto src2 = REG(fi.get_rs2());
        dst = src2 + (addr_t(uint32_t(src1)) << 1);
        NEXT_INSTR();
      } else PEPP_UNREACHABLE();
    }
    case RV64I_BC_OP_SH2ADD_UW: {
      if constexpr (W >= 8) {
        VIEW_INSTR_AS(fi, FasterOpType);
        auto &dst = REG(fi.get_rd());
        const auto src1 = REG(fi.get_rs1());
        const auto src2 = REG(fi.get_rs2());
        dst = src2 + (addr_t(uint32_t(src1)) << 2);
        NEXT_INSTR();
      } else PEPP_UNREACHABLE();
    }
#endif // RISCV_64I

    case RV32I_BC_OP_DIV: {
      VIEW_INSTR_AS(fi, FasterOpType);
      auto &dst = REG(fi.get_rd());
      const auto src1 = REG(fi.get_rs1());
      const auto src2 = REG(fi.get_rs2());
      // division by zero is not an exception
      if (LIKELY(saddr_t(src2) != 0)) {
        if constexpr (sizeof(address_t) == 8) {
          // vi_instr.cpp:444:2: runtime error:
          // division of -9223372036854775808 by -1 cannot be represented in type 'long'
          if (LIKELY(!((int64_t)src1 == INT64_MIN && (int64_t)src2 == -1ll))) dst = saddr_t(src1) / saddr_t(src2);
        } else {
          // rv32i_instr.cpp:301:2: runtime error:
          // division of -2147483648 by -1 cannot be represented in type 'int'
          if (LIKELY(!(src1 == 2147483648 && src2 == 4294967295))) dst = saddr_t(src1) / saddr_t(src2);
        }
      } else {
        dst = addr_t(-1);
      }
      NEXT_INSTR();
    }
    case RV32I_BC_OP_DIVU: {
      VIEW_INSTR_AS(fi, FasterOpType);
      auto &dst = REG(fi.get_rd());
      const auto src1 = REG(fi.get_rs1());
      const auto src2 = REG(fi.get_rs2());
      if (LIKELY(src2 != 0)) dst = src1 / src2;
      else dst = addr_t(-1);

      NEXT_INSTR();
    }
    case RV32I_BC_OP_REM: {
      VIEW_INSTR_AS(fi, FasterOpType);
      auto &dst = REG(fi.get_rd());
      const auto src1 = REG(fi.get_rs1());
      const auto src2 = REG(fi.get_rs2());
      if (LIKELY(src2 != 0)) {
        if constexpr (sizeof(address_t) == 4) {
          if (LIKELY(!(src1 == 2147483648 && src2 == 4294967295))) dst = saddr_t(src1) % saddr_t(src2);
        } else if constexpr (sizeof(address_t) == 8) {
          if (LIKELY(!((int64_t)src1 == INT64_MIN && (int64_t)src2 == -1ll))) dst = saddr_t(src1) % saddr_t(src2);
        } else dst = saddr_t(src1) % saddr_t(src2);
      }
      NEXT_INSTR();
    }
    case RV32I_BC_OP_REMU: {
      VIEW_INSTR_AS(fi, FasterOpType);
      auto &dst = REG(fi.get_rd());
      const auto src1 = REG(fi.get_rs1());
      const auto src2 = REG(fi.get_rs2());
      if (LIKELY(src2 != 0)) dst = src1 % src2;
      else dst = addr_t(-1);
      NEXT_INSTR();
    }

    case RV32I_BC_LDB: {
      VIEW_INSTR_AS(fi, FasterItype);
      const auto addr = REG(fi.get_rs2()) + fi.signed_imm();
      REG(fi.get_rs1()) = int8_t((*this).memory().template read<uint8_t>(addr));
      NEXT_INSTR();
    }
    case RV32I_BC_LDBU: {
      VIEW_INSTR_AS(fi, FasterItype);
      const auto addr = REG(fi.get_rs2()) + fi.signed_imm();
      REG(fi.get_rs1()) = saddr_t((*this).memory().template read<uint8_t>(addr));
      NEXT_INSTR();
    }
    case RV32I_BC_LDH: {
      VIEW_INSTR_AS(fi, FasterItype);
      const auto addr = REG(fi.get_rs2()) + fi.signed_imm();
      REG(fi.get_rs1()) = int16_t((*this).memory().template read<uint16_t>(addr));
      NEXT_INSTR();
    }
    case RV32I_BC_LDHU: {
      VIEW_INSTR_AS(fi, FasterItype);
      const auto addr = REG(fi.get_rs2()) + fi.signed_imm();
      REG(fi.get_rs1()) = saddr_t((*this).memory().template read<uint16_t>(addr));
      NEXT_INSTR();
    }
    case RV32I_BC_STB: {
      VIEW_INSTR_AS(fi, FasterItype);
      const auto addr = REG(fi.get_rs1()) + fi.signed_imm();
      (*this).memory().template write<uint8_t>(addr, REG(fi.get_rs2()));
      NEXT_INSTR();
    }
    case RV32I_BC_STH: {
      VIEW_INSTR_AS(fi, FasterItype);
      const auto addr = REG(fi.get_rs1()) + fi.signed_imm();
      (*this).memory().template write<uint16_t>(addr, REG(fi.get_rs2()));
      NEXT_INSTR();
    }
#ifdef RISCV_64I
    case RV32I_BC_LDWU: {
      if constexpr (W >= 8) {
        VIEW_INSTR_AS(fi, FasterItype);
        const auto addr = REG(fi.get_rs2()) + fi.signed_imm();
        REG(fi.get_rs1()) = (*this).memory().template read<uint32_t>(addr);
        NEXT_INSTR();
      } else PEPP_UNREACHABLE();
    }
#endif // RISCV_64I

#ifdef RISCV_EXT_COMPRESSED
    case RV32C_BC_SRLI: {
      VIEW_INSTR_AS(fi, FasterItype);
      REG(fi.get_rs1()) >>= fi.imm;
      NEXT_C_INSTR();
    }
    case RV32C_BC_ADD: {
      VIEW_INSTR_AS(fi, FasterItype);
      REG(fi.get_rs1()) += REG(fi.get_rs2());
      NEXT_C_INSTR();
    }
    case RV32C_BC_XOR: {
      VIEW_INSTR_AS(fi, FasterItype);
      REG(fi.get_rs1()) ^= REG(fi.get_rs2());
      NEXT_C_INSTR();
    }
    case RV32C_BC_OR: {
      VIEW_INSTR_AS(fi, FasterItype);
      REG(fi.get_rs1()) |= REG(fi.get_rs2());
      NEXT_C_INSTR();
    }
    case RV32C_BC_ANDI: {
      VIEW_INSTR_AS(fi, FasterItype);
      REG(fi.get_rs1()) &= fi.signed_imm();
      NEXT_C_INSTR();
    }
    case RV32C_BC_FUNCTION: {
      (*this).execute((*decoder).m_handler, (*decoder).instr);
      NEXT_C_INSTR();
    }
#endif

#ifdef RISCV_EXT_VECTOR
    case RV32V_BC_VLE32: {
      VIEW_INSTR_AS(vi, FasterMove);
      const auto &addr = REG(vi.rs1);
      registers().rvv().get(vi.rd) = (*this).memory().template read<VectorLane>(addr);
      NEXT_INSTR();
    }
    case RV32V_BC_VSE32: {
      VIEW_INSTR_AS(vi, FasterMove);
      const auto &addr = REG(vi.rs1);
      auto &value = registers().rvv().get(vi.rd);
      (*this).memory().template write<VectorLane>(addr, value);
      NEXT_INSTR();
    }
    case RV32V_BC_VFADD_VV: {
      VIEW_INSTR_AS(vi, FasterOpType);
      auto &rvv = registers().rvv();
      for (size_t i = 0; i < rvv.f32(0).size(); i++) {
        rvv.f32(vi.rd)[i] = rvv.f32(vi.rs1)[i] + rvv.f32(vi.rs2)[i];
      }
      NEXT_INSTR();
    }
    case RV32V_BC_VFMUL_VF: {
      VIEW_INSTR_AS(vi, FasterOpType);
      auto &rvv = registers().rvv();
      for (size_t i = 0; i < rvv.f32(0).size(); i++) {
        rvv.f32(vi.rd)[i] = rvv.f32(vi.rs2)[i] * registers().getfl(vi.rs1).f32[0];
      }
      NEXT_INSTR();
    }
#endif // RISCV_EXT_VECTOR

    case RV32I_BC_LIVEPATCH: {
      switch ((*decoder).m_handler) {
      case 0: { // Live-patch binary translation
        // Invalid handler
        (*decoder).set_bytecode(RV32I_BC_INVALID);
        (*decoder).set_invalid_handler();
      } break;
      case 1: { // Live-patch JALR -> STOP
        // Check if RA == memory exit address
        if (RISCV_SPECSAFE(REG(REG_RA) == machine().memory.exit_address())) {
          // Hot-swap the bytecode to a STOP
          (*decoder).set_bytecode(RV32I_BC_STOP);
          continue;
          ;
        }
        // Otherwise, leave the JALR instruction as is (NOTE: sets invalid handler)
        (*decoder).set_atomic_bytecode_and_handler(RV32I_BC_JALR, 0);
      } break;
#ifdef RISCV_EXT_COMPRESSED
      case 2: { // Live-patch C.JR -> STOP
        // Check if RA == memory exit address
        if (RISCV_SPECSAFE(REG(REG_RA) == machine().memory.exit_address())) {
          // Hot-swap the bytecode to a STOP
          (*decoder).set_bytecode(RV32I_BC_STOP);
          continue;
        }
        // Otherwise, leave the JR instruction as is (NOTE: sets invalid handler)
        (*decoder).set_atomic_bytecode_and_handler(RV32C_BC_JR, 0);
      } break;
#endif
      default:
        // Invalid handler
        (*decoder).set_bytecode(RV32I_BC_INVALID);
        (*decoder).set_invalid_handler();
      }
      continue;
    }

    case RV32I_BC_FUNCTION: {
      // printf("Slowpath: 0x%X  (instr: 0x%X)\n", uint32_t(pc), (*decoder).instr);
      (*this).execute((*decoder).m_handler, (*decoder).instr);
      NEXT_INSTR();
    }

    case RV32I_BC_FUNCBLOCK: {
      VIEW_INSTR();
      (*this).execute((*decoder).m_handler, (*decoder).instr);
      NEXT_BLOCK(instr.length(), true);
    }

    case RV32I_BC_SYSTEM: {
      VIEW_INSTR();
      // Make the current PC visible
      registers().pc = pc;
      // Make the instruction counters visible
      counter.apply(machine());
      // Invoke SYSTEM
      machine().system(instr);
      // Restore counters
      counter.retrieve_counters(machine());
      if (UNLIKELY(counter.overflowed() || pc != registers().pc)) {
        pc = registers().pc;
        goto check_jump;
      }
      // Overflow-check, next block
      NEXT_BLOCK(4, true);
    }

    case RV32I_BC_SYSCALL: {
      // Make the current PC visible
      registers().pc = pc;
      // Make the instruction counter(s) visible
      counter.apply(machine());
      // Invoke system call
      machine().system_call(REG(REG_ECALL));
      // Restore counters
      counter.retrieve_counters(machine());
      if (UNLIKELY(counter.overflowed() || pc != registers().pc)) {
        // System calls are always full-length instructions
        if constexpr (VERBOSE_JUMPS) {
          if (pc != registers().pc)
            fprintf(stderr, "SYSCALL jump from 0x%lX to 0x%lX\n", long(pc), long(registers().pc + 4));
        }
        pc = registers().pc + 4;
        goto check_jump;
      }
      NEXT_BLOCK(4, false);
    }
    case RV32I_BC_STOP: {
      registers().pc = pc + 4;
      machine().set_instruction_counter(counter.value());
      return true;
    }

    default: goto execute_invalid;
    } // switch case
  } // while loop

check_jump:
  if (UNLIKELY(counter.overflowed())) goto counter_overflow;

  if (LIKELY(pc - current_begin < current_end - current_begin)) goto continue_segment;
  else goto new_execute_segment;

counter_overflow:
  registers().pc = pc;
  machine().set_instruction_counter(counter.value());

  // Machine stopped normally?
  return counter.max() == 0;

  // Change to a new execute segment
new_execute_segment: {
  auto new_values = this->next_execute_segment(pc);
  exec = new_values.exec;
  pc = new_values.pc;
  current_begin = exec->exec_begin();
  current_end = exec->exec_end();
  exec_decoder = exec->decoder_cache();
}
  goto continue_segment;

execute_invalid:
  // Calculate the current PC from the decoder pointer
  pc = (decoder - exec_decoder) << DecoderCache<address_t>::SHIFT;
  // Check if the instruction is still invalid
  try {
    if (decoder->instr == 0 && machine().memory.template read<uint16_t>(pc) != 0) {
      exec->set_stale(true);
      goto new_execute_segment;
    }
  } catch (...) {
  }
  machine().set_instruction_counter(counter.value());
  registers().pc = pc;
  trigger_exception(ILLEGAL_OPCODE, decoder->instr);

} // CPU::simulate_XXX()
} // namespace riscv
