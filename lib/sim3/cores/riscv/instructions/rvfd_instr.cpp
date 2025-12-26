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
#include "sim3/common_macros.hpp"
#include <cmath>
#include "sim3/systems/notraced_riscv_isa3_system.hpp"
#include "./rvfd_instr.hpp"
#include "instr_helpers.hpp"

namespace riscv
{
	// RISC-V Canonical NaNs
	static constexpr uint32_t CANONICAL_NAN_F32 = 0x7fc00000;
	static constexpr uint64_t CANONICAL_NAN_F64 = 0x7ff8000000000000;

	template <typename T>
	static bool is_signaling_nan(T t) {
		if constexpr (sizeof(T) == 4)
			return (*(uint32_t*)&t & 0x7fa00000) == 0x7f800000;
		else
			return (*(uint64_t*)&t & 0x7ffe000000000000) == 0x7ff0000000000000;
	}

#ifdef RISCV_FCSR
	template <int W, typename T>
	static void fsflags(CPU<address_t>& cpu, long double exact, T& inexact) {
		if constexpr (fcsr_emulation) {
			auto& fcsr = cpu.registers().fcsr();
			fcsr.fflags = 0;
			if (std::isnan(exact) || std::isnan(inexact)) {
				fcsr.fflags |= 16;
				// Canonical NaN
				if constexpr (sizeof(T) == 4)
					*(int32_t *)&inexact = CANONICAL_NAN_F32;
				else
					*(int64_t *)&inexact = CANONICAL_NAN_F64;
			} else {
				if (exact != inexact) fcsr.fflags |= 1;
			}
		}
	}
#else
#define fsflags(c, e, i) /**/
#endif
  template <bool Signaling, AddressType address_t, typename T, typename R>
  static void feqflags(CPU<address_t> &cpu, T a, T b, R &dst) {
    if constexpr (fcsr_emulation) {
			auto& fcsr = cpu.registers().fcsr();
			fcsr.fflags = 0;
			if (std::isnan(a) || std::isnan(b)) {
				// All operations return 0 when either operand is NaN
				dst = 0;
			}
			if constexpr (Signaling) {
				if (std::isnan(a) || std::isnan(b))
					fcsr.fflags |= 16;
			} else { // Quiet
				if (is_signaling_nan(a) || is_signaling_nan(b))
					fcsr.fflags |= 16;
			}
		}
  }

  template <AddressType address_t> RVINSTR_ATTR void FLW_handler(CPU<address_t> &cpu, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};
    auto addr = cpu.reg(fi.Itype.rs1) + fi.Itype.signed_imm();
    auto &dst = cpu.registers().getfl(fi.Itype.rd);
    dst.load_u32(cpu.machine().memory.template read<uint32_t>(addr));
  };
  template <AddressType address_t>
  RVPRINTR_ATTR int FLW_printer(char *buffer, size_t len, const CPU<address_t> &, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};
    static const std::array<const char *, 8> insn{"???", "FLH", "FLW", "FLD", "FLQ", "???", "???", "???"};
    return snprintf(buffer, len, "%s %s, [%s%+d]", insn[fi.Itype.funct3], RISCV::flpname(fi.Itype.rd),
                    RISCV::regname(fi.Stype.rs1), fi.Itype.signed_imm());
  };
  template <AddressType address_t> RVINSTR_ATTR void FLD_handler(CPU<address_t> &cpu, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};
    auto addr = cpu.reg(fi.Itype.rs1) + fi.Itype.signed_imm();
    auto &dst = cpu.registers().getfl(fi.Itype.rd);
    dst.load_u64(cpu.machine().memory.template read<uint64_t>(addr));
  };

  template <AddressType address_t> RVINSTR_ATTR void FSW_handler(CPU<address_t> &cpu, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};
    const auto &src = cpu.registers().getfl(fi.Stype.rs2);
    auto addr = cpu.reg(fi.Stype.rs1) + fi.Stype.signed_imm();
    cpu.machine().memory.template write<uint32_t>(addr, src.i32[0]);
  };
  template <AddressType address_t>
  RVPRINTR_ATTR int FSW_printer(char *buffer, size_t len, const CPU<address_t> &, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};
    static const std::array<const char *, 8> insn{"???", "FSH", "FSW", "FSD", "FSQ", "???", "???", "???"};
    return snprintf(buffer, len, "%s [%s%+d], %s", insn[fi.Stype.funct3], RISCV::regname(fi.Stype.rs1),
                    fi.Stype.signed_imm(), RISCV::flpname(fi.Stype.rs2));
  };
  template <AddressType address_t> RVINSTR_ATTR void FSD_handler(CPU<address_t> &cpu, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};
    const auto &src = cpu.registers().getfl(fi.Stype.rs2);
    auto addr = cpu.reg(fi.Stype.rs1) + fi.Stype.signed_imm();
    cpu.machine().memory.template write<uint64_t>(addr, src.i64);
  };

  template <AddressType address_t> RVINSTR_ATTR void FMADD_handler(CPU<address_t> &cpu, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};
    auto &dst = cpu.registers().getfl(fi.R4type.rd);
    auto &rs1 = cpu.registers().getfl(fi.R4type.rs1);
    auto &rs2 = cpu.registers().getfl(fi.R4type.rs2);
    auto &rs3 = cpu.registers().getfl(fi.R4type.rs3);
    if (fi.R4type.funct2 == 0x0) { // float32
      dst.set_float(rs1.f32[0] * rs2.f32[0] + rs3.f32[0]);
      fsflags(cpu, (double)rs1.f32[0] * (double)rs2.f32[0] + (double)rs3.f32[0], dst.f32[0]);
    } else if (fi.R4type.funct2 == 0x1) { // float64
      dst.f64 = rs1.f64 * rs2.f64 + rs3.f64;
      fsflags(cpu, (long double)rs1.f64 * (long double)rs2.f64 + (long double)rs3.f64, dst.f64);
    } else {
      cpu.trigger_exception(ILLEGAL_OPERATION);
    }
  };
  template <AddressType address_t>
  RVPRINTR_ATTR int FMADD_printer(char *buffer, size_t len, const CPU<address_t> &, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};
    static const std::array<const char *, 4> f2{"FMADD.S", "FMADD.D", "???", "FMADD.Q"};
    return snprintf(buffer, len, "%s %s * %s + %s, %s", f2[fi.R4type.funct2], RISCV::flpname(fi.R4type.rs1),
                    RISCV::flpname(fi.R4type.rs2), RISCV::flpname(fi.R4type.rs3), RISCV::flpname(fi.R4type.rd));
  };

  template <AddressType address_t> RVINSTR_ATTR void FMSUB_handler(CPU<address_t> &cpu, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};
    auto &dst = cpu.registers().getfl(fi.R4type.rd);
    auto &rs1 = cpu.registers().getfl(fi.R4type.rs1);
    auto &rs2 = cpu.registers().getfl(fi.R4type.rs2);
    auto &rs3 = cpu.registers().getfl(fi.R4type.rs3);
    if (fi.R4type.funct2 == 0x0) { // float32
      dst.set_float(rs1.f32[0] * rs2.f32[0] - rs3.f32[0]);
      fsflags(cpu, (double)rs1.f32[0] * (double)rs2.f32[0] - (double)rs3.f32[0], dst.f32[0]);
    } else if (fi.R4type.funct2 == 0x1) { // float64
      dst.f64 = rs1.f64 * rs2.f64 - rs3.f64;
      fsflags(cpu, (long double)rs1.f64 * (long double)rs2.f64 - (long double)rs3.f64, dst.f64);
    } else {
      cpu.trigger_exception(ILLEGAL_OPERATION);
    }
  };
  template <AddressType address_t>
  RVPRINTR_ATTR int FMSUB_printer(char *buffer, size_t len, const CPU<address_t> &, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};
    static const std::array<const char *, 4> f2{"FMSUB.S", "FMSUB.D", "???", "FMSUB.Q"};
    return snprintf(buffer, len, "%s %s * %s - %s, %s", f2[fi.R4type.funct2], RISCV::flpname(fi.R4type.rs1),
                    RISCV::flpname(fi.R4type.rs2), RISCV::flpname(fi.R4type.rs3), RISCV::flpname(fi.R4type.rd));
  };

  template <AddressType address_t> RVINSTR_ATTR void FNMADD_handler(CPU<address_t> &cpu, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};
    auto &dst = cpu.registers().getfl(fi.R4type.rd);
    auto &rs1 = cpu.registers().getfl(fi.R4type.rs1);
    auto &rs2 = cpu.registers().getfl(fi.R4type.rs2);
    auto &rs3 = cpu.registers().getfl(fi.R4type.rs3);
    if (fi.R4type.funct2 == 0x0) { // float32
      dst.set_float(-(rs1.f32[0] * rs2.f32[0]) - rs3.f32[0]);
      fsflags(cpu, (double)-rs1.f32[0] * (double)rs2.f32[0] - (double)rs3.f32[0], dst.f32[0]);
    } else if (fi.R4type.funct2 == 0x1) { // float64
      dst.f64 = -(rs1.f64 * rs2.f64) - rs3.f64;
      fsflags(cpu, (long double)-rs1.f64 * (long double)rs2.f64 - (long double)rs3.f64, dst.f64);
    } else {
      cpu.trigger_exception(ILLEGAL_OPERATION);
    }
  };
  template <AddressType address_t>
  RVPRINTR_ATTR int FNMADD_printer(char *buffer, size_t len, const CPU<address_t> &, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};
    static const std::array<const char *, 4> f2{"FMADD.S", "FMADD.D", "???", "FMADD.Q"};
    return snprintf(buffer, len, "%s %s %s, %s", f2[fi.R4type.funct2], RISCV::flpname(fi.R4type.rs1),
                    RISCV::flpname(fi.R4type.rs2), RISCV::flpname(fi.R4type.rd));
  };

  template <AddressType address_t> RVINSTR_ATTR void FNMSUB_handler(CPU<address_t> &cpu, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};
    auto &dst = cpu.registers().getfl(fi.R4type.rd);
    auto &rs1 = cpu.registers().getfl(fi.R4type.rs1);
    auto &rs2 = cpu.registers().getfl(fi.R4type.rs2);
    auto &rs3 = cpu.registers().getfl(fi.R4type.rs3);
    if (fi.R4type.funct2 == 0x0) { // float32
      dst.set_float(-(rs1.f32[0] * rs2.f32[0]) + rs3.f32[0]);
      fsflags(cpu, (double)-rs1.f32[0] * (double)rs2.f32[0] + (double)rs3.f32[0], dst.f32[0]);
    } else if (fi.R4type.funct2 == 0x1) { // float64
      dst.f64 = -(rs1.f64 * rs2.f64) + rs3.f64;
      fsflags(cpu, (long double)-rs1.f64 * (long double)rs2.f64 + (long double)rs3.f64, dst.f64);
    } else {
      cpu.trigger_exception(ILLEGAL_OPERATION);
    }
  };
  template <AddressType address_t>
  RVPRINTR_ATTR int FNMSUB_printer(char *buffer, size_t len, const CPU<address_t> &, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};
    static const std::array<const char *, 4> f2{"FNMSUB.S", "FNMSUB.D", "???", "FNMSUB.Q"};
    return snprintf(buffer, len, "%s -(%s * %s) + %s, %s", f2[fi.R4type.funct2], RISCV::flpname(fi.R4type.rs1),
                    RISCV::flpname(fi.R4type.rs2), RISCV::flpname(fi.R4type.rs3), RISCV::flpname(fi.R4type.rd));
  };

  template <AddressType address_t> RVINSTR_ATTR void FADD_handler(CPU<address_t> &cpu, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};
    auto &dst = cpu.registers().getfl(fi.R4type.rd);
    auto &rs1 = cpu.registers().getfl(fi.R4type.rs1);
    auto &rs2 = cpu.registers().getfl(fi.R4type.rs2);
    if (fi.R4type.funct2 == 0x0) { // float32
      dst.set_float(rs1.f32[0] + rs2.f32[0]);
      fsflags(cpu, (double)(rs1.f32[0]) + (double)(rs2.f32[0]), dst.f32[0]);
    } else if (fi.R4type.funct2 == 0x1) { // float64
      dst.f64 = rs1.f64 + rs2.f64;
      fsflags(cpu, (long double)(rs1.f64) + (long double)(rs2.f64), dst.f64);
    } else {
      cpu.trigger_exception(ILLEGAL_OPERATION);
    }
  };
  template <AddressType address_t>
  RVPRINTR_ATTR int FADD_printer(char *buffer, size_t len, const CPU<address_t> &, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};
    static const std::array<const char *, 4> f2{"FADD.S", "FADD.D", "???", "FADD.Q"};
    return snprintf(buffer, len, "%s %s %s, %s", f2[fi.R4type.funct2], RISCV::flpname(fi.R4type.rs1),
                    RISCV::flpname(fi.R4type.rs2), RISCV::flpname(fi.R4type.rd));
  };

  template <AddressType address_t> RVINSTR_ATTR void FSUB_handler(CPU<address_t> &cpu, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};
    auto &dst = cpu.registers().getfl(fi.R4type.rd);
    auto &rs1 = cpu.registers().getfl(fi.R4type.rs1);
    auto &rs2 = cpu.registers().getfl(fi.R4type.rs2);
    if (fi.R4type.funct2 == 0x0) { // float32
      dst.set_float(rs1.f32[0] - rs2.f32[0]);
      fsflags(cpu, (double)(rs1.f32[0]) - (double)(rs2.f32[0]), dst.f32[0]);
    } else if (fi.R4type.funct2 == 0x1) { // float64
      dst.f64 = rs1.f64 - rs2.f64;
      fsflags(cpu, (long double)(rs1.f64) - (long double)(rs2.f64), dst.f64);
    } else {
      cpu.trigger_exception(ILLEGAL_OPERATION);
    }
  };
  template <AddressType address_t>
  RVPRINTR_ATTR int FSUB_printer(char *buffer, size_t len, const CPU<address_t> &, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};
    static const std::array<const char *, 4> f2{"FSUB.S", "FSUB.D", "???", "FSUB.Q"};
    return snprintf(buffer, len, "%s %s %s, %s", f2[fi.R4type.funct2], RISCV::flpname(fi.R4type.rs1),
                    RISCV::flpname(fi.R4type.rs2), RISCV::flpname(fi.R4type.rd));
  };

  template <AddressType address_t> RVINSTR_ATTR void FMUL_handler(CPU<address_t> &cpu, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};
    auto &dst = cpu.registers().getfl(fi.R4type.rd);
    auto &rs1 = cpu.registers().getfl(fi.R4type.rs1);
    auto &rs2 = cpu.registers().getfl(fi.R4type.rs2);
    if (fi.R4type.funct2 == 0x0) { // float32
      dst.set_float(rs1.f32[0] * rs2.f32[0]);
      fsflags(cpu, (double)(rs1.f32[0]) * (double)(rs2.f32[0]), dst.f32[0]);
    } else if (fi.R4type.funct2 == 0x1) { // float64
      dst.f64 = rs1.f64 * rs2.f64;
      fsflags(cpu, (long double)(rs1.f64) * (long double)(rs2.f64), dst.f64);
    } else {
      cpu.trigger_exception(ILLEGAL_OPERATION);
    }
  };
  template <AddressType address_t>
  RVPRINTR_ATTR int FMUL_printer(char *buffer, size_t len, const CPU<address_t> &, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};
    static const std::array<const char *, 4> f2{"FMUL.S", "FMUL.D", "???", "FMUL.Q"};
    return snprintf(buffer, len, "%s %s %s, %s", f2[fi.R4type.funct2], RISCV::flpname(fi.R4type.rs1),
                    RISCV::flpname(fi.R4type.rs2), RISCV::flpname(fi.R4type.rd));
  };

  template <AddressType address_t> RVINSTR_ATTR void FDIV_handler(CPU<address_t> &cpu, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};
    auto &dst = cpu.registers().getfl(fi.R4type.rd);
    auto &rs1 = cpu.registers().getfl(fi.R4type.rs1);
    auto &rs2 = cpu.registers().getfl(fi.R4type.rs2);
    if (fi.R4type.funct2 == 0x0) { // fp32
      dst.set_float(rs1.f32[0] / rs2.f32[0]);
      fsflags(cpu, (double)(rs1.f32[0]) / (double)(rs2.f32[0]), dst.f32[0]);
    } else if (fi.R4type.funct2 == 0x1) { // fp64
      dst.f64 = rs1.f64 / rs2.f64;
      fsflags(cpu, (long double)(rs1.f64) / (long double)(rs2.f64), dst.f64);
    } else {
      cpu.trigger_exception(ILLEGAL_OPERATION);
    }
  };
  template <AddressType address_t>
  RVPRINTR_ATTR int FDIV_printer(char *buffer, size_t len, const CPU<address_t> &, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};
    static const std::array<const char *, 4> f2{"FDIV.S", "FDIV.D", "???", "FDIV.Q"};
    return snprintf(buffer, len, "%s %s %s, %s", f2[fi.R4type.funct2], RISCV::flpname(fi.R4type.rs1),
                    RISCV::flpname(fi.R4type.rs2), RISCV::flpname(fi.R4type.rd));
  };

  template <AddressType address_t> RVINSTR_ATTR void FSQRT_handler(CPU<address_t> &cpu, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};
    auto &rs1 = cpu.registers().getfl(fi.R4type.rs1);
    auto &dst = cpu.registers().getfl(fi.R4type.rd);
    switch (fi.R4type.funct2) {
    case 0x0: // FSQRT.S
      dst.set_float(sqrtf(rs1.f32[0]));
      fsflags(cpu, std::sqrt((double)(rs1.f32[0])), dst.f32[0]);
      break;
    case 0x1: // FSQRT.D
      dst.f64 = sqrt(rs1.f64);
      fsflags(cpu, std::sqrt((long double)(rs1.f64)), dst.f64);
      break;
    default: cpu.trigger_exception(ILLEGAL_OPERATION);
    }
  };
  template <AddressType address_t>
  RVPRINTR_ATTR int FSQRT_printer(char *buffer, size_t len, const CPU<address_t> &, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};
    static const std::array<const char *, 4> f2{"FSQRT.S", "FSQRT.D", "???", "FSQRT.Q"};
    return snprintf(buffer, len, "%s %s, %s", f2[fi.R4type.funct2], RISCV::flpname(fi.R4type.rs1),
                    RISCV::flpname(fi.R4type.rd));
  };

  template <AddressType address_t>
  RVINSTR_COLDATTR void FMIN_FMAX_handler(CPU<address_t> &cpu, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};
    auto &rs1 = cpu.registers().getfl(fi.R4type.rs1);
    auto &rs2 = cpu.registers().getfl(fi.R4type.rs2);
    auto &dst = cpu.registers().getfl(fi.R4type.rd);

    switch (fi.R4type.funct3 | (fi.R4type.funct2 << 4)) {
    case 0x0: // FMIN.S
      if constexpr (fcsr_emulation) {
        if (std::isnan(rs1.f32[0]) && std::isnan(rs2.f32[0])) dst.load_u32(CANONICAL_NAN_F32);
        else dst.set_float(std::fmin(rs1.f32[0], rs2.f32[0]));
      } else {
        dst.set_float(std::fmin(rs1.f32[0], rs2.f32[0]));
      }
      break;
    case 0x1: // FMAX.S
      if constexpr (fcsr_emulation) {
        if (std::isnan(rs1.f32[0]) && std::isnan(rs2.f32[0])) dst.load_u32(CANONICAL_NAN_F32);
        else dst.set_float(std::fmax(rs1.f32[0], rs2.f32[0]));
      } else {
        dst.set_float(std::fmax(rs1.f32[0], rs2.f32[0]));
      }
      break;
    case 0x10: // FMIN.D
      if constexpr (fcsr_emulation) {
        if (std::isnan(rs1.f64) && std::isnan(rs2.f64)) dst.load_u64(CANONICAL_NAN_F64);
        else dst.f64 = std::fmin(rs1.f64, rs2.f64);
      } else {
        dst.f64 = std::fmin(rs1.f64, rs2.f64);
      }
      break;
    case 0x11: // FMAX.D
      if constexpr (fcsr_emulation) {
        if (std::isnan(rs1.f64) && std::isnan(rs2.f64)) dst.load_u64(CANONICAL_NAN_F64);
        else dst.f64 = std::fmax(rs1.f64, rs2.f64);
      } else {
        dst.f64 = std::fmax(rs1.f64, rs2.f64);
      }
      break;
    default: cpu.trigger_exception(ILLEGAL_OPERATION);
    }
    if constexpr (fcsr_emulation) {
      if (is_signaling_nan(rs1.f32[0]) || is_signaling_nan(rs2.f32[0])) cpu.registers().fcsr().fflags = 16;
      else cpu.registers().fcsr().fflags = 0;
    }
  };
  template <AddressType address_t>
  RVPRINTR_ATTR int FMIN_FMAX_printer(char *buffer, size_t len, const CPU<address_t> &, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};
    static const std::array<const char *, 8> insn{"FMIN", "FMAX", "???", "???", "???", "???", "???", "???"};
    return snprintf(buffer, len, "%s.%c %s %s, %s", insn[fi.R4type.funct3], RISCV::flpsize(fi.R4type.funct2),
                    RISCV::flpname(fi.R4type.rs1), RISCV::flpname(fi.R4type.rs2), RISCV::regname(fi.R4type.rd));
  };

  template <AddressType address_t> RVINSTR_ATTR void FEQ_FLT_FLE_handler(CPU<address_t> &cpu, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};
    auto &rs1 = cpu.registers().getfl(fi.R4type.rs1);
    auto &rs2 = cpu.registers().getfl(fi.R4type.rs2);
    auto &dst = cpu.reg(fi.R4type.rd);

    switch (fi.R4type.funct3 | (fi.R4type.funct2 << 4)) {
    case 0x0: // FLE.S
      dst = (rs1.f32[0] <= rs2.f32[0]) ? 1 : 0;
      feqflags<true>(cpu, rs1.f32[0], rs2.f32[0], dst);
      break;
    case 0x1: // FLT.S
      dst = (rs1.f32[0] < rs2.f32[0]) ? 1 : 0;
      feqflags<true>(cpu, rs1.f32[0], rs2.f32[0], dst);
      break;
    case 0x2: // FEQ.S
      dst = (rs1.f32[0] == rs2.f32[0]) ? 1 : 0;
      feqflags<false>(cpu, rs1.f32[0], rs2.f32[0], dst);
      break;
    case 0x10: // FLE.D
      dst = (rs1.f64 <= rs2.f64) ? 1 : 0;
      feqflags<true>(cpu, rs1.f64, rs2.f64, dst);
      break;
    case 0x11: // FLT.D
      dst = (rs1.f64 < rs2.f64) ? 1 : 0;
      feqflags<true>(cpu, rs1.f64, rs2.f64, dst);
      break;
    case 0x12: // FEQ.D
      dst = (rs1.f64 == rs2.f64) ? 1 : 0;
      feqflags<false>(cpu, rs1.f64, rs2.f64, dst);
      break;
    default: cpu.trigger_exception(ILLEGAL_OPERATION);
    }
  };
  template <AddressType address_t>
  RVPRINTR_ATTR int FEQ_FLT_FLE_printer(char *buffer, size_t len, const CPU<address_t> &, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};
    static const std::array<const char *, 4> insn{"FLE", "FLT", "FEQ", "F???"};
    return snprintf(buffer, len, "%s.%c %s %s, %s", insn[fi.R4type.funct3], RISCV::flpsize(fi.R4type.funct2),
                    RISCV::flpname(fi.R4type.rs1), RISCV::flpname(fi.R4type.rs2), RISCV::regname(fi.R4type.rd));
  };

  template <AddressType address_t> RVINSTR_ATTR void FCVT_SD_DS_handler(CPU<address_t> &cpu, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};
    auto &rs1 = cpu.registers().getfl(fi.R4type.rs1);
    auto &dst = cpu.registers().getfl(fi.R4type.rd);
    switch (fi.R4type.funct2) {
    case 0x0: // FCVT.S.D (64 -> 32)
      dst.set_float(rs1.f64);
      break;
    case 0x1: // FCVT.D.S (32 -> 64)
      dst.f64 = rs1.f32[0];
      break;
    default: cpu.trigger_exception(ILLEGAL_OPERATION);
    }
  };
  template <AddressType address_t>
  RVPRINTR_ATTR int FCVT_SD_DS_printer(char *buffer, size_t len, const CPU<address_t> &, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};
    static const std::array<const char *, 4> f2{"FCVT.S.D", "FCVT.D.S", "???", "???"};
    return snprintf(buffer, len, "%s %s, %s", f2[fi.R4type.funct2], RISCV::flpname(fi.R4type.rs1),
                    RISCV::flpname(fi.R4type.rd));
  };

  template <AddressType address_t> RVINSTR_ATTR void FCVT_W_SD_handler(CPU<address_t> &cpu, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};
    auto &rs1 = cpu.registers().getfl(fi.R4type.rs1);
    auto &dst = cpu.reg(fi.R4type.rd);
    switch (fi.R4type.funct2) {
    case 0x0: // from float32
      if (fi.R4type.rs2 == 0x0) dst = (int32_t)rs1.f32[0];
      else dst = (uint32_t)rs1.f32[0];
      return;
    case 0x1: // from float64
      switch (fi.R4type.rs2) {
      case 0x0: // FCVT.W.D
        dst = (int32_t)rs1.f64;
        return;
      case 0x1: // FCVT.WU.D
        dst = (uint32_t)rs1.f64;
        return;
      case 0x2: // FCVT.L.D
        dst = (int64_t)rs1.f64;
        return;
      case 0x3: // FCVT.LU.D
        dst = (uint64_t)rs1.f64;
        return;
      }
    }
    cpu.trigger_exception(ILLEGAL_OPERATION);
  };
  template <AddressType address_t>
  RVPRINTR_ATTR int FCVT_W_SD_printer(char *buffer, size_t len, const CPU<address_t> &, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};
    static const std::array<const char *, 4> f2{"FCVT.W.S", "FCVT.W.D", "???", "FCVT.W.Q"};
    return snprintf(buffer, len, "%s %s, %s", f2[fi.R4type.funct2], RISCV::flpname(fi.R4type.rs1),
                    RISCV::regname(fi.R4type.rd));
  };

  template <AddressType address_t> RVINSTR_ATTR void FCVT_SD_W_handler(CPU<address_t> &cpu, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};
    auto &rs1 = cpu.reg(fi.R4type.rs1);
    auto &dst = cpu.registers().getfl(fi.R4type.rd);
    switch (fi.R4type.funct2) {
    case 0x0:                   // to float32
      if (fi.R4type.rs2 == 0x0) // FCVT.S.W
        dst.set_float((int32_t)rs1);
      else // FCVT.S.WU
        dst.set_float((uint32_t)rs1);
      return;
    case 0x1: // to float64
      switch (fi.R4type.rs2) {
      case 0x0: // FCVT.D.W
        dst.f64 = (int32_t)rs1;
        return;
      case 0x1: // FCVT.D.WU
        dst.f64 = (uint32_t)rs1;
        return;
      case 0x2: // FCVT.D.L
        dst.f64 = (int64_t)rs1;
        return;
      case 0x3: // FCVT.D.LU
        dst.f64 = (uint64_t)rs1;
        return;
      }
    }
    cpu.trigger_exception(ILLEGAL_OPERATION);
  };
  template <AddressType address_t>
  RVPRINTR_ATTR int FCVT_SD_W_printer(char *buffer, size_t len, const CPU<address_t> &, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};
    static const std::array<const char *, 4> f2{"FCVT.S.W", "FCVT.D.W", "???", "FCVT.Q.W"};
    return snprintf(buffer, len, "%s %s, %s", f2[fi.R4type.funct2], RISCV::regname(fi.R4type.rs1),
                    RISCV::flpname(fi.R4type.rd));
  };

  template <AddressType address_t> RVINSTR_ATTR void FSGNJ_NX_handler(CPU<address_t> &cpu, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};
    auto &rs1 = cpu.registers().getfl(fi.R4type.rs1);
    auto &rs2 = cpu.registers().getfl(fi.R4type.rs2);
    auto &dst = cpu.registers().getfl(fi.R4type.rd);
    switch (fi.R4type.funct3) {
    case 0x0: // FSGNJ
      switch (fi.R4type.funct2) {
      case 0x0: // float32
        dst.load_u32((rs2.lsign.sign << 31) | rs1.lsign.bits);
        break;
      case 0x1: // float64
        dst.i64 = ((uint64_t)rs2.usign.sign << 63) | rs1.usign.bits;
        break;
      default: cpu.trigger_exception(ILLEGAL_OPERATION);
      }
      break;
    case 0x1: // FSGNJ_N
      switch (fi.R4type.funct2) {
      case 0x0: // float32
        dst.load_u32((~rs2.lsign.sign << 31) | rs1.lsign.bits);
        break;
      case 0x1: // float64
        dst.i64 = (~(uint64_t)rs2.usign.sign << 63) | rs1.usign.bits;
        break;
      default: cpu.trigger_exception(ILLEGAL_OPERATION);
      }
      break;
    case 0x2: // FSGNJ_X
      switch (fi.R4type.funct2) {
      case 0x0: // float32
        dst.load_u32(((rs1.lsign.sign ^ rs2.lsign.sign) << 31) | rs1.lsign.bits);
        break;
      case 0x1: // float64
        dst.i64 = ((uint64_t)(rs1.usign.sign ^ rs2.usign.sign) << 63) | rs1.usign.bits;
        break;
      default: cpu.trigger_exception(ILLEGAL_OPERATION);
      }
      break;
    default: cpu.trigger_exception(ILLEGAL_OPERATION);
    }
  };
  template <AddressType address_t>
  RVPRINTR_ATTR int FSGNJ_NX_printer(char *buffer, size_t len, const CPU<address_t> &, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};

    if (fi.R4type.rs1 == fi.R4type.rs2) {
      static const char *insn[4] = {"FMV", "FNEG", "FABS", "???"};
      return snprintf(buffer, len, "%s.%c %s, %s", insn[fi.R4type.funct3], RISCV::flpsize(fi.R4type.funct2),
                      RISCV::flpname(fi.R4type.rs1), RISCV::flpname(fi.R4type.rd));
    }
    static const char *insn[4] = {"FSGNJ", "FSGNJN", "FSGNJX", "???"};
    return snprintf(buffer, len, "%s.%c %s %s, %s", insn[fi.R4type.funct3], RISCV::flpsize(fi.R4type.funct2),
                    RISCV::flpname(fi.R4type.rs1), RISCV::flpname(fi.R4type.rs2), RISCV::flpname(fi.R4type.rd));
  };

  // 1110 f3 = 0x1
  template <AddressType address_t> RVINSTR_ATTR void FCLASS_handler(CPU<address_t> &cpu, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};
    auto &dst = cpu.reg(fi.R4type.rd);
    auto &rs1 = cpu.registers().getfl(fi.R4type.rs1);
    switch (fi.R4type.funct2) {
    case 0x0: // FCLASS.S
      dst = 0;
      if (rs1.f32[0] == -std::numeric_limits<float>::infinity()) dst |= 1U << 0;
      if (rs1.f32[0] < 0) dst |= 1U << 1;
      if (rs1.f32[0] == -std::numeric_limits<float>::denorm_min()) dst |= 1U << 2;
      if (rs1.f32[0] == -0.0) dst |= 1U << 3;
      if (rs1.f32[0] == +0.0) dst |= 1U << 4;
      if (rs1.f32[0] == std::numeric_limits<float>::denorm_min()) dst |= 1U << 5;
      if (rs1.f32[0] >= std::numeric_limits<float>::epsilon()) dst |= 1U << 6;
      if (rs1.f32[0] == std::numeric_limits<float>::infinity()) dst |= 1U << 7;
      if (std::isnan(rs1.f32[0])) dst |= 3U << 8;
      return;
    case 0x1: // FCLASS.D
      dst = 0;
      if (rs1.f64 == -std::numeric_limits<double>::infinity()) dst |= 1U << 0;
      if (rs1.f64 < 0) dst |= 1U << 1;
      if (rs1.f64 == -std::numeric_limits<double>::denorm_min()) dst |= 1U << 2;
      if (rs1.f64 == -0.0) dst |= 1U << 3;
      if (rs1.f64 == +0.0) dst |= 1U << 4;
      if (rs1.f64 == std::numeric_limits<double>::denorm_min()) dst |= 1U << 5;
      if (rs1.f64 >= std::numeric_limits<double>::epsilon()) dst |= 1U << 6;
      if (rs1.f64 == std::numeric_limits<double>::infinity()) dst |= 1U << 7;
      if (std::isnan(rs1.f64)) dst |= 3U << 8;
      return;
    }
    cpu.trigger_exception(ILLEGAL_OPERATION);
  };
  template <AddressType address_t>
  RVPRINTR_ATTR int FCLASS_printer(char *buffer, size_t len, const CPU<address_t> &, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};
    static const std::array<const char *, 4> f2{"FCLASS.S", "FCLASS.D", "???", "FCLASS.Q"};
    return snprintf(buffer, len, "%s %s, %s", f2[fi.R4type.funct2], RISCV::flpname(fi.R4type.rs1),
                    RISCV::regname(fi.R4type.rd));
  };

  // 1110 f3 = 0x0
  template <AddressType address_t> RVINSTR_ATTR void FMV_X_W_handler(CPU<address_t> &cpu, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};
    auto &dst = cpu.reg(fi.R4type.rd);
    auto &rs1 = cpu.registers().getfl(fi.R4type.rs1);
    switch (fi.R4type.funct2) {
    case 0x0: // FMV.X.W
              // FMV.X.W moves the single-precision value in floating-point register rs1 represented in IEEE 754-
      // 2008 encoding to the lower 32 bits of integer register rd. The bits are not modified in the transfer,
      // and in particular, the payloads of non-canonical NaNs are preserved. For RV64, the higher 32 bits
      // of the destination register are filled with copies of the floating-point number’s sign bit.
      dst = RVSIGNTYPE(cpu)(rs1.i32[0]);
      return;
    case 0x1: // FMV.X.D
      if constexpr (RVISGE64BIT(cpu)) {
        dst = RVSIGNTYPE(cpu)(rs1.i64);
        return;
      }
      break;
    }
    cpu.trigger_exception(ILLEGAL_OPERATION);
  };
  template <AddressType address_t>
  RVPRINTR_ATTR int FMV_X_W_printer(char *buffer, size_t len, const CPU<address_t> &, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};
    static const std::array<const char *, 4> f2{"FMV.X.W", "FMV.X.D", "???", "FMV.X.Q"};
    return snprintf(buffer, len, "%s %s, %s", f2[fi.R4type.funct2], RISCV::flpname(fi.R4type.rs1),
                    RISCV::regname(fi.R4type.rd));
  };

  // 1111
  template <AddressType address_t> RVINSTR_ATTR void FMV_W_X_handler(CPU<address_t> &cpu, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};
    auto &rs1 = cpu.reg(fi.R4type.rs1);
    auto &dst = cpu.registers().getfl(fi.R4type.rd);
    switch (fi.R4type.funct2) {
    case 0x0: // FMV.W.X
      dst.load_u32(rs1);
      return;
    case 0x1: // FMV.D.X
      if constexpr (RVISGE64BIT(cpu)) {
        dst.load_u64(rs1);
        return;
      }
      break;
    }
    cpu.trigger_exception(ILLEGAL_OPERATION);
  };
  template <AddressType address_t>
  RVPRINTR_ATTR int FMV_W_X_printer(char *buffer, size_t len, const CPU<address_t> &, rv32i_instruction instr) {
    const rv32f_instruction fi{instr};
    static const std::array<const char *, 4> f2{"FMV.W.X", "FMV.D.X", "???", "FMV.Q.X"};
    return snprintf(buffer, len, "%s %s, %s", f2[fi.R4type.funct2], RISCV::regname(fi.R4type.rs1),
                    RISCV::flpname(fi.R4type.rd));
  };

  } // namespace riscv

  const riscv::Instruction<uint32_t> instr32i_FLW{riscv::FLW_handler, riscv::FLW_printer};
  const riscv::Instruction<uint32_t> instr32i_FLD{riscv::FLD_handler, riscv::FLW_printer};
  const riscv::Instruction<uint32_t> instr32i_FSW{riscv::FSW_handler, riscv::FSW_printer};
  const riscv::Instruction<uint32_t> instr32i_FSD{riscv::FSD_handler, riscv::FSW_printer};
  const riscv::Instruction<uint32_t> instr32i_FMADD{riscv::FMADD_handler, riscv::FMADD_printer};
  const riscv::Instruction<uint32_t> instr32i_FMSUB{riscv::FMSUB_handler, riscv::FMSUB_printer};
  const riscv::Instruction<uint32_t> instr32i_FNMADD{riscv::FNMADD_handler, riscv::FNMADD_printer};
  const riscv::Instruction<uint32_t> instr32i_FNMSUB{riscv::FNMSUB_handler, riscv::FNMSUB_printer};
  const riscv::Instruction<uint32_t> instr32i_FADD{riscv::FADD_handler, riscv::FADD_printer};
  const riscv::Instruction<uint32_t> instr32i_FSUB{riscv::FSUB_handler, riscv::FSUB_printer};
  const riscv::Instruction<uint32_t> instr32i_FMUL{riscv::FMUL_handler, riscv::FMUL_printer};
  const riscv::Instruction<uint32_t> instr32i_FDIV{riscv::FDIV_handler, riscv::FDIV_printer};
  const riscv::Instruction<uint32_t> instr32i_FSQRT{riscv::FSQRT_handler, riscv::FSQRT_printer};
  const riscv::Instruction<uint32_t> instr32i_FMIN_FMAX{riscv::FMIN_FMAX_handler, riscv::FMIN_FMAX_printer};
  const riscv::Instruction<uint32_t> instr32i_FEQ_FLT_FLE{riscv::FEQ_FLT_FLE_handler, riscv::FEQ_FLT_FLE_printer};
  const riscv::Instruction<uint32_t> instr32i_FCVT_SD_DS{riscv::FCVT_SD_DS_handler, riscv::FCVT_SD_DS_printer};
  const riscv::Instruction<uint32_t> instr32i_FCVT_W_SD{riscv::FCVT_W_SD_handler, riscv::FCVT_W_SD_printer};
  const riscv::Instruction<uint32_t> instr32i_FCVT_SD_W{riscv::FCVT_SD_W_handler, riscv::FCVT_SD_W_printer};
  const riscv::Instruction<uint32_t> instr32i_FSGNJ_NX{riscv::FSGNJ_NX_handler, riscv::FSGNJ_NX_printer};
  const riscv::Instruction<uint32_t> instr32i_FCLASS{riscv::FCLASS_handler, riscv::FCLASS_printer};
  const riscv::Instruction<uint32_t> instr32i_FMV_X_W{riscv::FMV_X_W_handler, riscv::FMV_X_W_printer};
  const riscv::Instruction<uint32_t> instr32i_FMV_W_X{riscv::FMV_W_X_handler, riscv::FMV_W_X_printer};

  const riscv::Instruction<uint64_t> instr64i_FLW{riscv::FLW_handler, riscv::FLW_printer};
  const riscv::Instruction<uint64_t> instr64i_FLD{riscv::FLD_handler, riscv::FLW_printer};
  const riscv::Instruction<uint64_t> instr64i_FSW{riscv::FSW_handler, riscv::FSW_printer};
  const riscv::Instruction<uint64_t> instr64i_FSD{riscv::FSD_handler, riscv::FSW_printer};
  const riscv::Instruction<uint64_t> instr64i_FMADD{riscv::FMADD_handler, riscv::FMADD_printer};
  const riscv::Instruction<uint64_t> instr64i_FMSUB{riscv::FMSUB_handler, riscv::FMSUB_printer};
  const riscv::Instruction<uint64_t> instr64i_FNMADD{riscv::FNMADD_handler, riscv::FNMADD_printer};
  const riscv::Instruction<uint64_t> instr64i_FNMSUB{riscv::FNMSUB_handler, riscv::FNMSUB_printer};
  const riscv::Instruction<uint64_t> instr64i_FADD{riscv::FADD_handler, riscv::FADD_printer};
  const riscv::Instruction<uint64_t> instr64i_FSUB{riscv::FSUB_handler, riscv::FSUB_printer};
  const riscv::Instruction<uint64_t> instr64i_FMUL{riscv::FMUL_handler, riscv::FMUL_printer};
  const riscv::Instruction<uint64_t> instr64i_FDIV{riscv::FDIV_handler, riscv::FDIV_printer};
  const riscv::Instruction<uint64_t> instr64i_FSQRT{riscv::FSQRT_handler, riscv::FSQRT_printer};
  const riscv::Instruction<uint64_t> instr64i_FMIN_FMAX{riscv::FMIN_FMAX_handler, riscv::FMIN_FMAX_printer};
  const riscv::Instruction<uint64_t> instr64i_FEQ_FLT_FLE{riscv::FEQ_FLT_FLE_handler, riscv::FEQ_FLT_FLE_printer};
  const riscv::Instruction<uint64_t> instr64i_FCVT_SD_DS{riscv::FCVT_SD_DS_handler, riscv::FCVT_SD_DS_printer};
  const riscv::Instruction<uint64_t> instr64i_FCVT_W_SD{riscv::FCVT_W_SD_handler, riscv::FCVT_W_SD_printer};
  const riscv::Instruction<uint64_t> instr64i_FCVT_SD_W{riscv::FCVT_SD_W_handler, riscv::FCVT_SD_W_printer};
  const riscv::Instruction<uint64_t> instr64i_FSGNJ_NX{riscv::FSGNJ_NX_handler, riscv::FSGNJ_NX_printer};
  const riscv::Instruction<uint64_t> instr64i_FCLASS{riscv::FCLASS_handler, riscv::FCLASS_printer};
  const riscv::Instruction<uint64_t> instr64i_FMV_X_W{riscv::FMV_X_W_handler, riscv::FMV_X_W_printer};
  const riscv::Instruction<uint64_t> instr64i_FMV_W_X{riscv::FMV_W_X_handler, riscv::FMV_W_X_printer};
