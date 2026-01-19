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
#include "rvv_instr.hpp"
#include "bts/isa/riscv/rvv.hpp"
#include "instr_helpers.hpp"
#include "sim3/common_macros.hpp"
#include "sim3/cores/riscv/rvv_registers.hpp"
#include "sim3/subsystems/ram/paged_pool.hpp"
#include "sim3/systems/notraced_riscv_isa3_system.hpp"

namespace riscv
{
	static const char *VOPNAMES[3][64] = {
		{"VADD", "???", "VSUB", "VRSUB", "VMINU", "VMIN", "VMAXU", "VMAX", "???", "VAND", "VOR", "VXOR", "VRGATHER", "???", "VSLIDEUP", "VSLIDEDOWN",
		 "VADC", "VMADC", "VSBC", "VMSBC", "???", "???", "???", "VMERGE", "???", "???", "???", "???", "???", "???", "???", "???"
		 "VSADDU", "VSADD", "VSSUBU", "VSSUB", "???", "VSLL", "???", "VSMUL", "VSRL", "VSRA", "VSSRL", "VSSRA", "VNSLR", "VNSRA", "VNCLIPU", "VNCLIP",
		 "VWREDSUMU", "VWREDSUM", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???"},
		{"VREDSUM", "VREDAND", "VREDOR", "VREDXOR", "VREDMINU", "VREDMIN", "VREDMAXU", "VREDMAX", "VAADDU", "VAADD", "VASUBU", "VASUB", "???", "???", "VSLIDE1UP", "VSLIDE1DOWN",
		 "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???", "???"
		 "VDIVU", "VDIV", "VREMU", "VREM", "VMULHU", "VMUL", "VMULHSU", "VMULH", "???", "VMADD", "???", "VNMSUB", "???", "VMACC", "VNMSAC",
		  "VWADDU", "VWADD", "VWSUBU", "VWSUB", "VWADDU.W", "VWADD.W", "VWSUBU.W", "VWSUB.W", "VWMULU", "???", "VWMULSU", "VWMUL", "VWMACCU", "VWMACC", "VWMACCUS", "VWMACCSU"},
		{"VFADD", "VFREDUSUM", "VFSUB", "VFREDOSUM", "VFMIN", "VFREDMIN", "VFMAX", "VFREDMAX", "VFSGNJ", "VFSGNJ.N", "VFSGNJ.X", "???", "???", "???", "VFSLIDE1UP", "VFSLIDE1DOWN",
		 "VWFUNARY0", "???", "VFUNARY0", "VFUNARY1", "???", "???", "???", "VFMERGE", "VMFEQ", "MVFLE", "???", "VMFLT", "VMFNE", "VMFGT", "???", "VMFGE",
		 "VFDIV", "VFRDIV", "???", "???", "VFMUL", "???", "???", "VFRSUB", "VFMADD", "VFNMADD", "VFMSUB", "VFNMSUB", "VFMACC", "VFNMACC", "VFMSAC", "VFNMSAC",
		 "VFWADD", "VFWREDUSUM", "VFWSUB", "VFWREDOSUM", "VFWADD.W", "???", "VFWSUB.W", "???", "VFWMUL", "???", "???", "???", "VFWMACC", "VFWNMACC", "VFWMSAC", "VFWNMSAC"},
		};

  template <AddressType address_t> RVINSTR_ATTR void VSETVLI_handler(CPU<address_t> &cpu, rv32i_instruction instr) {
    const rv32v_instruction vi{instr};
    cpu.trigger_exception(UNIMPLEMENTED_INSTRUCTION);
  };
  template <AddressType address_t>
  RVPRINTR_ATTR int VSETVLI_printer(char *buffer, size_t len, const CPU<address_t> &, rv32i_instruction instr) {
    const rv32v_instruction vi{instr};
    return snprintf(buffer, len, "VSETVLI %s, %s, 0x%X", RISCV::regname(vi.VLI.rd), RISCV::regname(vi.VLI.rs1),
                    vi.VLI.zimm);
  };

  template <AddressType address_t> RVINSTR_ATTR void VSETIVLI_handler(CPU<address_t> &cpu, rv32i_instruction instr) {
    const rv32v_instruction vi{instr};
    cpu.trigger_exception(UNIMPLEMENTED_INSTRUCTION);
  };
  template <AddressType address_t>
  RVPRINTR_ATTR int VSETIVLI_printer(char *buffer, size_t len, const CPU<address_t> &, rv32i_instruction instr) {
    const rv32v_instruction vi{instr};
    return snprintf(buffer, len, "VSETIVLI %s, 0x%X, 0x%X", RISCV::regname(vi.IVLI.rd), vi.IVLI.uimm, vi.IVLI.zimm);
  };

  template <AddressType address_t> RVINSTR_ATTR void VSETVL_handler(CPU<address_t> &cpu, rv32i_instruction instr) {
    const rv32v_instruction vi{instr};
    cpu.trigger_exception(UNIMPLEMENTED_INSTRUCTION);
  };
  template <AddressType address_t>
  RVPRINTR_ATTR int VSETVL_printer(char *buffer, size_t len, const CPU<address_t> &, rv32i_instruction instr) {
    const rv32v_instruction vi{instr};
    return snprintf(buffer, len, "VSETVL %s, %s, %s", RISCV::regname(vi.VSETVL.rd), RISCV::regname(vi.VSETVL.rs1),
                    RISCV::regname(vi.VSETVL.rs2));
  };

  template <AddressType address_t> RVINSTR_ATTR void VLE32_handler(CPU<address_t> &cpu, rv32i_instruction instr) {
    const rv32v_instruction vi{instr};
    const auto addr = cpu.reg(vi.VLS.rs1);
    if (riscv::force_align_memory || addr % VectorLane::size() == 0) {
      auto &rvv = cpu.registers().rvv();
      rvv.get(vi.VLS.vd) = cpu.machine().memory.template read<VectorLane>(addr);
    } else {
      cpu.trigger_exception(INVALID_ALIGNMENT, addr);
    }
  };
  template <AddressType address_t>
  RVPRINTR_ATTR int VLE32_printer(char *buffer, size_t len, const CPU<address_t> &, rv32i_instruction instr) {
    const rv32v_instruction vi{instr};
    return snprintf(buffer, len, "VLE32.V %s, %s, %s", RISCV::vecname(vi.VLS.vd), RISCV::regname(vi.VLS.rs1),
                    RISCV::regname(vi.VLS.rs2));
  };

  template <AddressType address_t> RVINSTR_ATTR void VSE32_handler(CPU<address_t> &cpu, rv32i_instruction instr) {
    const rv32v_instruction vi{instr};
    const auto addr = cpu.reg(vi.VLS.rs1);
    if (riscv::force_align_memory || addr % VectorLane::size() == 0) {
      auto &rvv = cpu.registers().rvv();
      cpu.machine().memory.template write<VectorLane>(addr, rvv.get(vi.VLS.vd));
    } else {
      cpu.trigger_exception(INVALID_ALIGNMENT, addr);
    }
  };
  template <AddressType address_t>
  RVPRINTR_ATTR int VSE32_printer(char *buffer, size_t len, const CPU<address_t> &, rv32i_instruction instr) {
    const rv32v_instruction vi{instr};
    return snprintf(buffer, len, "VSE32.V %s, %s, %s", RISCV::vecname(vi.VLS.vd), RISCV::regname(vi.VLS.rs1),
                    RISCV::regname(vi.VLS.rs2));
  };

  template <AddressType address_t> RVINSTR_ATTR void VOPI_VV_handler(CPU<address_t> &cpu, rv32i_instruction instr) {
    const rv32v_instruction vi{instr};
    auto &rvv = cpu.registers().rvv();
    switch (vi.OPVV.funct6) {
    case 0b000000: // VADD
      for (size_t i = 0; i < rvv.u32(0).size(); i++) {
        rvv.u32(vi.OPVV.vd)[i] = rvv.u32(vi.OPVV.vs1)[i] + rvv.u32(vi.OPVV.vs2)[i];
      }
      break;
    case 0b000010: // VSUB
      for (size_t i = 0; i < rvv.u32(0).size(); i++) {
        rvv.u32(vi.OPVV.vd)[i] = rvv.u32(vi.OPVV.vs1)[i] - rvv.u32(vi.OPVV.vs2)[i];
      }
      break;
    case 0b001001: // VAND
      for (size_t i = 0; i < rvv.u32(0).size(); i++) {
        rvv.u32(vi.OPVV.vd)[i] = rvv.u32(vi.OPVV.vs1)[i] & rvv.u32(vi.OPVV.vs2)[i];
      }
      break;
    case 0b001010: // VOR
      for (size_t i = 0; i < rvv.u32(0).size(); i++) {
        rvv.u32(vi.OPVV.vd)[i] = rvv.u32(vi.OPVV.vs1)[i] | rvv.u32(vi.OPVV.vs2)[i];
      }
      break;
    case 0b001011: // VXOR
      for (size_t i = 0; i < rvv.u32(0).size(); i++) {
        rvv.u32(vi.OPVV.vd)[i] = rvv.u32(vi.OPVV.vs1)[i] ^ rvv.u32(vi.OPVV.vs2)[i];
      }
      break;
    case 0b001100: // VRGATHER
      for (size_t i = 0; i < rvv.u32(0).size(); i++) {
        const auto vs1 = rvv.u32(vi.OPVV.vs1)[i];
        rvv.u32(vi.OPVV.vd)[i] = (vs1 >= rvv.u32(0).size()) ? 0 : rvv.u32(vi.OPVV.vs2)[vs1];
      }
      break;
    default: cpu.trigger_exception(UNIMPLEMENTED_INSTRUCTION);
    }
  };
  template <AddressType address_t>
  RVPRINTR_ATTR int VOPI_VV_printer(char *buffer, size_t len, const CPU<address_t> &, rv32i_instruction instr) {
    const rv32v_instruction vi{instr};
    return snprintf(buffer, len, "%s %s, %s, %s", VOPNAMES[0][vi.OPVV.funct6], RISCV::vecname(vi.VLS.vd),
                    RISCV::regname(vi.VLS.rs1), RISCV::regname(vi.VLS.rs2));
  };

  template <AddressType address_t> RVINSTR_ATTR void VOPF_VV_handler(CPU<address_t> &cpu, rv32i_instruction instr) {
    const rv32v_instruction vi{instr};
    auto &rvv = cpu.registers().rvv();
    switch (vi.OPVV.funct6) {
    case 0b000000: // VFADD.VV
      for (size_t i = 0; i < rvv.f32(0).size(); i++) {
        rvv.f32(vi.OPVV.vd)[i] = rvv.f32(vi.OPVV.vs1)[i] + rvv.f32(vi.OPVV.vs2)[i];
      }
      return;
    case 0b000001:   // VFREDUSUM
    case 0b000011: { // VFREDOSUM
      float sum = 0.0f;
      for (size_t i = 0; i < rvv.f32(0).size(); i++) {
        sum += rvv.f32(vi.OPVV.vs1)[i] + rvv.f32(vi.OPVV.vs2)[i];
      }
      rvv.f32(vi.OPVV.vd)[0] = sum;
    }
      return;
    case 0b010000:                  // VWUNARY0.VV
      if (vi.OPVV.vs1 == 0b00000) { // VFMV.F.S
        cpu.registers().getfl(vi.OPVV.vd).set_float(rvv.f32(vi.OPVV.vs2)[0]);
        return;
      }
      break;
    case 0b000010: // VFSUB.VV
      for (size_t i = 0; i < rvv.f32(0).size(); i++) {
        rvv.f32(vi.OPVV.vd)[i] = rvv.f32(vi.OPVV.vs1)[i] - rvv.f32(vi.OPVV.vs2)[i];
      }
      return;
    case 0b100100: // VFMUL.VV
      for (size_t i = 0; i < rvv.f32(0).size(); i++) {
        rvv.f32(vi.OPVV.vd)[i] = rvv.f32(vi.OPVV.vs1)[i] * rvv.f32(vi.OPVV.vs2)[i];
      }
      return;
    case 0b101000: // VFMADD.VV: Multiply-add (overwrites multiplicand)
      for (size_t i = 0; i < rvv.f32(0).size(); i++) {
        rvv.f32(vi.OPVV.vd)[i] = (rvv.f32(vi.OPVV.vs1)[i] * rvv.f32(vi.OPVV.vd)[i]) + rvv.f32(vi.OPVV.vs2)[i];
      }
      return;
    case 0b101100: // VFMACC.VV: Multiply-accumulate (overwrites addend)
      for (size_t i = 0; i < rvv.f32(0).size(); i++) {
        rvv.f32(vi.OPVV.vd)[i] = (rvv.f32(vi.OPVV.vs1)[i] * rvv.f32(vi.OPVV.vs2)[i]) + rvv.f32(vi.OPVV.vd)[i];
      }
      return;
    }
    cpu.trigger_exception(UNIMPLEMENTED_INSTRUCTION);
  };
  template <AddressType address_t>
  RVPRINTR_ATTR int VOPF_VV_printer(char *buffer, size_t len, const CPU<address_t> &, rv32i_instruction instr) {
    const rv32v_instruction vi{instr};
    return snprintf(buffer, len, "%s.VV %s, %s, %s", VOPNAMES[2][vi.OPVV.funct6], RISCV::vecname(vi.OPVV.vd),
                    RISCV::vecname(vi.OPVV.vs1), RISCV::vecname(vi.OPVV.vs2));
  };

  template <AddressType address_t> RVINSTR_ATTR void VOPM_VV_handler(CPU<address_t> &cpu, rv32i_instruction instr) {
    const rv32v_instruction vi{instr};
    cpu.trigger_exception(UNIMPLEMENTED_INSTRUCTION);
  };
  template <AddressType address_t>
  RVPRINTR_ATTR int VOPM_VV_printer(char *buffer, size_t len, const CPU<address_t> &, rv32i_instruction instr) {
    const rv32v_instruction vi{instr};
    return snprintf(buffer, len, "VOPM.VV %s, %s, %s", RISCV::vecname(vi.VLS.vd), RISCV::regname(vi.VLS.rs1),
                    RISCV::regname(vi.VLS.rs2));
  };

  template <AddressType address_t> RVINSTR_ATTR void VOPI_VI_handler(CPU<address_t> &cpu, rv32i_instruction instr) {
    const rv32v_instruction vi{instr};
    auto &rvv = cpu.registers().rvv();
    const uint32_t scalar = vi.OPVI.imm;
    switch (vi.OPVV.funct6) {
    case 0b010111: // VMERGE.VI
      if (vi.OPVI.vs2 == 0) {
        for (size_t i = 0; i < rvv.u32(0).size(); i++) {
          rvv.u32(vi.OPVI.vd)[i] = scalar;
        }
        return;
      }
    }
    cpu.trigger_exception(UNIMPLEMENTED_INSTRUCTION);
  };
  template <AddressType address_t>
  RVPRINTR_ATTR int VOPI_VI_printer(char *buffer, size_t len, const CPU<address_t> &, rv32i_instruction instr) {
    const rv32v_instruction vi{instr};
    return snprintf(buffer, len, "VOPI.VI %s %s, %s, %s", VOPNAMES[0][vi.OPVI.funct6], RISCV::vecname(vi.VLS.vd),
                    RISCV::regname(vi.VLS.rs1), RISCV::regname(vi.VLS.rs2));
  };

  template <AddressType address_t> RVINSTR_ATTR void VOPF_VF_handler(CPU<address_t> &cpu, rv32i_instruction instr) {
    const rv32v_instruction vi{instr};
    auto &rvv = cpu.registers().rvv();
    const float scalar = cpu.registers().getfl(vi.OPVV.vs1).f32[0];
    const auto vector = vi.OPVV.vs2;
    switch (vi.OPVV.funct6) {
    case 0b000000: // VFADD.VF
      for (size_t i = 0; i < rvv.f32(0).size(); i++) {
        rvv.f32(vi.OPVV.vd)[i] = rvv.f32(vector)[i] + scalar;
      }
      return;
    case 0b000001:   // VFREDUSUM.VF
    case 0b000011: { // VFREDOSUM.VF
      float sum = 0.0f;
      for (size_t i = 0; i < rvv.f32(0).size(); i++) {
        sum += rvv.f32(vector)[i] + scalar;
      }
      rvv.f32(vi.OPVV.vd)[0] = sum;
    }
      return;
    case 0b000010: // VFSUB.VF
      for (size_t i = 0; i < rvv.f32(0).size(); i++) {
        rvv.f32(vi.OPVV.vd)[i] = rvv.f32(vector)[i] - scalar;
      }
      return;
    case 0b010000:       // VRFUNARY0.VF
      if (vector == 0) { // VFMV.S.F
        for (size_t i = 0; i < rvv.f32(0).size(); i++) {
          rvv.f32(vi.OPVV.vd)[i] = scalar;
        }
        return;
      }
      break;
    case 0b100100: // VFMUL.VF
      for (size_t i = 0; i < rvv.f32(0).size(); i++) {
        rvv.f32(vi.OPVV.vd)[i] = rvv.f32(vector)[i] * scalar;
      }
      return;
    }
    cpu.trigger_exception(UNIMPLEMENTED_INSTRUCTION);
  };
  template <AddressType address_t>
  RVPRINTR_ATTR int VOPF_VF_printer(char *buffer, size_t len, const CPU<address_t> &, rv32i_instruction instr) {
    const rv32v_instruction vi{instr};
    return snprintf(buffer, len, "VOPF.VF %s, %s, %s", RISCV::vecname(vi.VLS.vd), RISCV::regname(vi.VLS.rs1),
                    RISCV::regname(vi.VLS.rs2));
  };

  } // namespace riscv

  const riscv::Instruction<uint32_t> instr32i_VSETVLI{riscv::VSETVLI_handler, riscv::VSETVLI_printer};
  const riscv::Instruction<uint32_t> instr32i_VSETIVLI{riscv::VSETIVLI_handler, riscv::VSETIVLI_printer};
  const riscv::Instruction<uint32_t> instr32i_VSETVL{riscv::VSETVL_handler, riscv::VSETVL_printer};
  const riscv::Instruction<uint32_t> instr32i_VLE32{riscv::VLE32_handler, riscv::VLE32_printer};
  const riscv::Instruction<uint32_t> instr32i_VSE32{riscv::VSE32_handler, riscv::VSE32_printer};
  const riscv::Instruction<uint32_t> instr32i_VOPI_VV{riscv::VOPI_VV_handler, riscv::VOPI_VV_printer};
  const riscv::Instruction<uint32_t> instr32i_VOPF_VV{riscv::VOPF_VV_handler, riscv::VOPF_VV_printer};
  const riscv::Instruction<uint32_t> instr32i_VOPM_VV{riscv::VOPM_VV_handler, riscv::VOPM_VV_printer};
  const riscv::Instruction<uint32_t> instr32i_VOPI_VI{riscv::VOPI_VI_handler, riscv::VOPI_VI_printer};
  const riscv::Instruction<uint32_t> instr32i_VOPF_VF{riscv::VOPF_VF_handler, riscv::VOPF_VF_printer};

  const riscv::Instruction<uint64_t> instr64i_VSETVLI{riscv::VSETVLI_handler, riscv::VSETVLI_printer};
  const riscv::Instruction<uint64_t> instr64i_VSETIVLI{riscv::VSETIVLI_handler, riscv::VSETIVLI_printer};
  const riscv::Instruction<uint64_t> instr64i_VSETVL{riscv::VSETVL_handler, riscv::VSETVL_printer};
  const riscv::Instruction<uint64_t> instr64i_VLE32{riscv::VLE32_handler, riscv::VLE32_printer};
  const riscv::Instruction<uint64_t> instr64i_VSE32{riscv::VSE32_handler, riscv::VSE32_printer};
  const riscv::Instruction<uint64_t> instr64i_VOPI_VV{riscv::VOPI_VV_handler, riscv::VOPI_VV_printer};
  const riscv::Instruction<uint64_t> instr64i_VOPF_VV{riscv::VOPF_VV_handler, riscv::VOPF_VV_printer};
  const riscv::Instruction<uint64_t> instr64i_VOPM_VV{riscv::VOPM_VV_handler, riscv::VOPM_VV_printer};
  const riscv::Instruction<uint64_t> instr64i_VOPI_VI{riscv::VOPI_VI_handler, riscv::VOPI_VI_printer};
  const riscv::Instruction<uint64_t> instr64i_VOPF_VF{riscv::VOPF_VF_handler, riscv::VOPF_VF_printer};
#
