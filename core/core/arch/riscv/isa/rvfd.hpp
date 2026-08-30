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
#include <cstdint>
#include "core/arch/riscv/isa/rvi.hpp"

namespace riscv {

/*
 * F/D/Q instruction formats.
 *
 * Only two formats here are structurally new. The FP loads (FLW/FLD) and stores (FSW/FSD)
 * reuse the base integer formats verbatim, so decode them as InstructionI and InstructionS
 * from rvi.hpp -- those are bit-identical to the copies this header used to carry, down to
 * the reassembled immediate.
 */

// Rounding mode, held in funct3 of the arithmetic OP-FP and R4 encodings.
// Not every OP-FP instruction spends funct3 this way: the comparisons (FEQ/FLT/FLE), FCLASS,
// and the FMV pair overload it as an operation selector. Read it as a rounding mode only for
// instructions the spec says carry one, which is why funct3 keeps its raw name below and rm()
// is an opt-in accessor rather than the field itself.
enum class FpRm : uint8_t {
  RNE = 0b000, // Round to nearest, ties to even.
  RTZ = 0b001, // Round towards zero.
  RDN = 0b010, // Round down, towards -infinity.
  RUP = 0b011, // Round up, towards +infinity.
  RMM = 0b100, // Round to nearest, ties to max magnitude.
  // 0b101 and 0b110 are reserved.
  DYN = 0b111, // Defer to fcsr.frm.
};

// Operand width. Occupies funct2 of the R4 format, and the low two bits of funct7 on OP-FP.
enum class FpFmt : uint8_t {
  S = 0b00, // 32-bit single.
  D = 0b01, // 64-bit double.
  H = 0b10, // 16-bit half.
  Q = 0b11, // 128-bit quad.
};

// Fused multiply-add: FMADD, FMSUB, FNMSUB, FNMADD. The only RISC-V format with three source
// registers, which is why it claims four major opcodes of its own rather than sharing OP-FP.
struct InstructionR4 {
  uint32_t opcode : 7;
  uint32_t rd : 5;
  uint32_t funct3 : 3;
  uint32_t rs1 : 5;
  uint32_t rs2 : 5;
  uint32_t fmt : 2;
  uint32_t rs3 : 5;

  FpRm rm() const noexcept { return static_cast<FpRm>(funct3); }
  FpFmt format() const noexcept { return static_cast<FpFmt>(fmt); }
};
static_assert(sizeof(InstructionR4) == 4, "R4-type instruction must be 32 bits");

// Everything under the OP-FP major opcode. Structurally an R-type, but the spec splits funct7
// into a 5-bit operation selector and the 2-bit operand width, so the two are named separately
// here rather than making every consumer shift funct7 apart itself.
struct InstructionRFP {
  uint32_t opcode : 7;
  uint32_t rd : 5;
  uint32_t funct3 : 3;
  uint32_t rs1 : 5;
  uint32_t rs2 : 5;
  uint32_t fmt : 2;
  uint32_t funct5 : 5;

  FpRm rm() const noexcept { return static_cast<FpRm>(funct3); }
  FpFmt format() const noexcept { return static_cast<FpFmt>(fmt); }
};
static_assert(sizeof(InstructionRFP) == 4, "OP-FP R-type instruction must be 32 bits");

// Accrued exception flags, the low five bits of fcsr.
enum fflags {
  FFLAG_NX = 0x1,
  FFLAG_UF = 0x2,
  FFLAG_OF = 0x4,
  FFLAG_DZ = 0x8,
  FFLAG_NV = 0x10,
};
} // namespace riscv
