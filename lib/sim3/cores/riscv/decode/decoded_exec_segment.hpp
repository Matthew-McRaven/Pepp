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
#include <memory>
#include <stdexcept>
#include "../notraced_cpu.hpp"
#include "./threaded_bytecodes.hpp"
#include "bts/isa/rv_base.hpp"
#include "bts/isa/rv_types.hpp"
#include "bts/isa/rvc.hpp"
#include "bts/isa/rvfd.hpp"
#include "bts/isa/rvi.hpp"
#include "bts/isa/rvv.hpp"
#include "sim3/common_macros.hpp"

namespace riscv
{
	template<AddressType address_t> struct DecoderCache;
	template<AddressType address_t> struct DecoderData;

	// A fully decoded execute segment
	template <AddressType address_t>
	struct DecodedExecuteSegment
	{
		bool is_within(address_t addr, size_t len = 2) const noexcept {
			address_t addr_end;
#ifdef _MSC_VER
			addr_end = addr + len;
			return addr >= m_vaddr_begin && addr_end <= m_vaddr_end && (addr_end > addr);
#else
			if (!__builtin_add_overflow(addr, len, &addr_end))
				return addr >= m_vaddr_begin && addr_end <= m_vaddr_end;
#endif
			return false;
		}

		auto* exec_data(address_t pc = 0) const noexcept {
			return m_exec_pagedata.get() - m_exec_pagedata_base + pc;
		}

		address_t exec_begin() const noexcept { return m_vaddr_begin; }
		address_t exec_end() const noexcept { return m_vaddr_end; }
		address_t pagedata_base() const noexcept { return m_exec_pagedata_base; }

		auto* decoder_cache() noexcept { return m_exec_decoder; }
		auto* decoder_cache() const noexcept { return m_exec_decoder; }
		auto* decoder_cache_base() const noexcept { return m_decoder_cache.get(); }
		size_t decoder_cache_size() const noexcept { return m_decoder_cache_size; }

		auto* create_decoder_cache(DecoderCache<address_t>* cache, size_t size) {
			m_decoder_cache.reset(cache);
			m_decoder_cache_size = size;
			return m_decoder_cache.get();
		}
		void set_decoder(DecoderData<address_t>* dec) { m_exec_decoder = dec; }

		size_t size_bytes() const noexcept {
			return sizeof(*this) + m_exec_pagedata_size + m_decoder_cache_size; // * sizeof(DecoderCache<address_t>);
		}
		bool empty() const noexcept { return m_exec_pagedata_size == 0; }

		DecodedExecuteSegment() = default;
		DecodedExecuteSegment(address_t pbase, size_t len, address_t vaddr, size_t exlen);
		DecodedExecuteSegment(DecodedExecuteSegment&&);
		~DecodedExecuteSegment();

		size_t threaded_rewrite(size_t bytecode, address_t pc, rv32i_instruction& instr);

		uint32_t crc32c_hash() const noexcept { return m_crc32c_hash; }
		void set_crc32c_hash(uint32_t hash) { m_crc32c_hash = hash; }

    bool is_binary_translated() const noexcept { return false; }
		bool is_libtcc() const noexcept { return false; }

		bool is_execute_only() const noexcept { return m_is_execute_only; }
		void set_execute_only(bool is_xo) { m_is_execute_only = is_xo; }

		bool is_likely_jit() const noexcept { return m_is_likely_jit; }
		void set_likely_jit(bool is_jit) { m_is_likely_jit = is_jit; }

		bool is_stale() const noexcept { return m_is_stale; }
		void set_stale(bool is_stale) { m_is_stale = is_stale; }

	private:
		address_t m_vaddr_begin = 0;
		address_t m_vaddr_end   = 0;
		DecoderData<address_t>* m_exec_decoder = nullptr;

		// The flat execute segment is used to execute
		// the CPU::simulate_precise function in order to
		// support debugging, as well as when producing
		// the decoder cache
		size_t    m_exec_pagedata_size = 0;
		address_t m_exec_pagedata_base = 0;
		std::unique_ptr<uint8_t[]> m_exec_pagedata = nullptr;

		// Decoder cache is used to run bytecode simulation at a high speed
		size_t          m_decoder_cache_size = 0;
		std::unique_ptr<DecoderCache<address_t>[]> m_decoder_cache = nullptr;

		uint32_t m_crc32c_hash = 0x0; // CRC32-C of the execute segment
		bool m_is_execute_only = false;
		// High-memory execute segments are likely to be JIT'd, and needs to
		// be nuked when attempting to re-use the segment
		bool m_is_likely_jit = false;
		bool m_is_stale = false;
	};

	template <AddressType address_t>
	inline DecodedExecuteSegment<address_t>::DecodedExecuteSegment(
		address_t pbase, size_t len, address_t exaddr, size_t exlen)
	{
		m_vaddr_begin = exaddr;
		m_vaddr_end   = exaddr + exlen;
		m_exec_pagedata.reset(new uint8_t[len]);
		m_exec_pagedata_size = len;
		m_exec_pagedata_base = pbase;
	}

	template <AddressType address_t>
	inline DecodedExecuteSegment<address_t>::DecodedExecuteSegment(DecodedExecuteSegment&& other)
	{
		m_vaddr_begin = other.m_vaddr_begin;
		m_vaddr_end   = other.m_vaddr_end;
		m_exec_decoder = other.m_exec_decoder;
		other.m_exec_decoder = nullptr;

		m_exec_pagedata_size = other.m_exec_pagedata_size;
		m_exec_pagedata_base = other.m_exec_pagedata_base;
		m_exec_pagedata = std::move(other.m_exec_pagedata);

		m_decoder_cache_size = other.m_decoder_cache_size;
		m_decoder_cache = std::move(other.m_decoder_cache);
	}

  template <AddressType address_t> inline DecodedExecuteSegment<address_t>::~DecodedExecuteSegment() {}

  template <AddressType address_t>
  RISCV_INTERNAL size_t DecodedExecuteSegment<address_t>::threaded_rewrite(size_t bytecode, [[maybe_unused]] address_t pc,
                                                                   rv32i_instruction &instr) {
    static constexpr unsigned PCAL = compressed_enabled ? 2 : 4;
    static constexpr unsigned XLEN = 8 * sizeof(address_t);
    const auto &original = instr;

    switch (bytecode) {
    case RV32I_BC_INVALID:
    case RV32I_BC_FUNCTION:
    case RV32I_BC_FUNCBLOCK:
    case RV32I_BC_STOP:
    case RV32I_BC_SYSTEM: {
      // These bytecodes are already fast, no need to rewrite
      return bytecode;
    }
    case RV32I_BC_LUI:
    case RV32I_BC_AUIPC: {
      FasterJtype rewritten;
      rewritten.rd = original.Utype.rd;
      rewritten.offset = original.Utype.imm << 4;

      instr.whole = rewritten.whole;
      return bytecode;
    }
    case RV32I_BC_MV: {
      FasterMove rewritten;
      rewritten.rd = original.Itype.rd;
      rewritten.rs1 = original.Itype.rs1;

      instr.whole = rewritten.whole;
      return bytecode;
    }
    case RV32I_BC_LI: {
      FasterImmediate rewritten;
      rewritten.rd = original.Itype.rd;
      rewritten.imm = original.Itype.signed_imm();

      instr.whole = rewritten.whole;
      return bytecode;
    }
    case RV64I_BC_SLLIW:
    case RV64I_BC_SRLIW:
    case RV64I_BC_SRAIW: {
      if (sizeof(address_t) == 4) return RV32I_BC_INVALID;

      FasterItype rewritten;
      rewritten.rs1 = original.Itype.rd;
      rewritten.rs2 = original.Itype.rs1;
      rewritten.imm = original.Itype.imm & 31;

      instr.whole = rewritten.whole;
      return bytecode;
    }
    case RV32I_BC_SLLI:
    case RV32I_BC_SRLI:
    case RV32I_BC_SRAI:
    case RV32I_BC_BSETI:
    case RV32I_BC_BEXTI: {
      FasterItype rewritten;
      rewritten.rs1 = original.Itype.rd;
      rewritten.rs2 = original.Itype.rs1;
      rewritten.imm = original.Itype.imm & (XLEN - 1);

      instr.whole = rewritten.whole;
      return bytecode;
    }
    case RV64I_BC_ADDIW:
      if (sizeof(address_t) == 4) return RV32I_BC_INVALID;
      [[fallthrough]];
    case RV32I_BC_SEXT_B:
    case RV32I_BC_SEXT_H:
    case RV32I_BC_ADDI:
    case RV32I_BC_SLTI:
    case RV32I_BC_SLTIU:
    case RV32I_BC_XORI:
    case RV32I_BC_ORI:
    case RV32I_BC_ANDI: {
      FasterItype rewritten;
      rewritten.rs1 = original.Itype.rd;
      rewritten.rs2 = original.Itype.rs1;
      rewritten.imm = original.Itype.signed_imm();

      instr.whole = rewritten.whole;
      return bytecode;
    }
    case RV32I_BC_BEQ:
    case RV32I_BC_BNE:
    case RV32I_BC_BLT:
    case RV32I_BC_BGE:
    case RV32I_BC_BLTU:
    case RV32I_BC_BGEU: {
      const int32_t imm = original.Btype.signed_imm();
      address_t addr = 0;
#ifdef _MSC_VER
      addr = pc + imm;
      const bool overflow = false;
#else
      const bool overflow = __builtin_add_overflow(pc, imm, &addr);
#endif

      if (!this->is_within(addr, 4) || (addr % PCAL) != 0 || overflow) {
        // Use invalid instruction for out-of-bounds branches
        // or misaligned jumps. It is strictly a cheat, but
        // it should also never happen on (especially) these
        // instructions. No sandbox harm.
        return RV32I_BC_INVALID;
      }

      FasterItype rewritten;
      rewritten.rs1 = original.Btype.rs1;
      rewritten.rs2 = original.Btype.rs2;
      rewritten.imm = original.Btype.signed_imm();

      instr.whole = rewritten.whole;

      // Forward branches can skip instr count check
      if (imm > 0 && bytecode == RV32I_BC_BEQ) return RV32I_BC_BEQ_FW;
      if (imm > 0 && bytecode == RV32I_BC_BNE) return RV32I_BC_BNE_FW;

      return bytecode;
    }
    case RV64I_BC_OP_ADDW:
    case RV64I_BC_OP_SUBW:
    case RV64I_BC_OP_MULW:
    case RV64I_BC_OP_ADD_UW:
    case RV64I_BC_OP_SH1ADD_UW:
    case RV64I_BC_OP_SH2ADD_UW:
      if (sizeof(address_t) == 4) return RV32I_BC_INVALID;
      [[fallthrough]];
    case RV32I_BC_OP_ADD:
    case RV32I_BC_OP_SUB:
    case RV32I_BC_OP_SLL:
    case RV32I_BC_OP_SLT:
    case RV32I_BC_OP_SLTU:
    case RV32I_BC_OP_XOR:
    case RV32I_BC_OP_SRL:
    case RV32I_BC_OP_SRA:
    case RV32I_BC_OP_OR:
    case RV32I_BC_OP_AND:
    case RV32I_BC_OP_MUL:
    case RV32I_BC_OP_DIV:
    case RV32I_BC_OP_DIVU:
    case RV32I_BC_OP_REM:
    case RV32I_BC_OP_REMU:
    case RV32I_BC_OP_ZEXT_H:
    case RV32I_BC_OP_SH1ADD:
    case RV32I_BC_OP_SH2ADD:
    case RV32I_BC_OP_SH3ADD: {
      FasterOpType rewritten;
      rewritten.rd = original.Rtype.rd;
      rewritten.rs1 = original.Rtype.rs1;
      rewritten.rs2 = original.Rtype.rs2;

      instr.whole = rewritten.whole;
      return bytecode;
    }
    case RV32I_BC_LDWU:
    case RV32I_BC_LDD:
      if (sizeof(address_t) == 4) return RV32I_BC_INVALID;
      [[fallthrough]];
    case RV32I_BC_LDB:
    case RV32I_BC_LDBU:
    case RV32I_BC_LDH:
    case RV32I_BC_LDHU:
    case RV32I_BC_LDW: {
      FasterItype rewritten;
      rewritten.rs1 = original.Itype.rd;
      rewritten.rs2 = original.Itype.rs1;
      rewritten.imm = original.Itype.signed_imm();

      instr.whole = rewritten.whole;
      return bytecode;
    }
    case RV32I_BC_STD:
      if (sizeof(address_t) == 4) return RV32I_BC_INVALID;
      [[fallthrough]];
    case RV32I_BC_STB:
    case RV32I_BC_STH:
    case RV32I_BC_STW: {
      FasterItype rewritten;
      rewritten.rs1 = original.Stype.rs1;
      rewritten.rs2 = original.Stype.rs2;
      rewritten.imm = original.Stype.signed_imm();

      instr.whole = rewritten.whole;
      return bytecode;
    }
    case RV32F_BC_FLW:
    case RV32F_BC_FLD: {
      const rv32f_instruction fi{original};
      FasterItype rewritten;
      rewritten.rs1 = fi.Itype.rd;
      rewritten.rs2 = fi.Itype.rs1;
      rewritten.imm = fi.Itype.signed_imm();

      instr.whole = rewritten.whole;
      return bytecode;
    }
    case RV32F_BC_FSW:
    case RV32F_BC_FSD: {
      const rv32f_instruction fi{original};
      FasterItype rewritten;
      rewritten.rs1 = fi.Stype.rs1;
      rewritten.rs2 = fi.Stype.rs2;
      rewritten.imm = fi.Stype.signed_imm();

      instr.whole = rewritten.whole;
      return bytecode;
    }
    case RV32I_BC_JAL: {
      const auto addr = pc + original.Jtype.jump_offset();
      const bool is_aligned = addr % PCAL == 0;
      const bool store_zero = original.Jtype.rd == 0;
      const bool store_ra = original.Jtype.rd == REG_RA;

      // The destination address also needs to be within
      // the current execute segment, as an optimization.
      if (this->is_within(addr, 4) && is_aligned) {
        const int32_t diff = addr - pc;
        if (!this->is_within(pc + diff, 4)) {
          return RV32I_BC_INVALID;
        } else if (store_zero) {
          instr.whole = diff;
          return RV32I_BC_FAST_JAL;
        } else if (store_ra) {
          // TODO: Optimize forward JALs instead
          instr.whole = diff;
          return RV32I_BC_FAST_CALL;
        }

        FasterJtype rewritten;
        rewritten.offset = original.Jtype.jump_offset();
        rewritten.rd = original.Jtype.rd;

        instr.whole = rewritten.whole;
        return bytecode;
      }

      return RV32I_BC_INVALID;
    }
    case RV32I_BC_JALR: {
      FasterItype rewritten;
      rewritten.imm = original.Itype.signed_imm();
      rewritten.rs1 = original.Itype.rd;
      rewritten.rs2 = original.Itype.rs1;

      instr.whole = rewritten.whole;
      return bytecode;
    }
    /** FP 32- and 64-bit instructions **/
    case RV32F_BC_FADD:
    case RV32F_BC_FSUB:
    case RV32F_BC_FMUL:
    case RV32F_BC_FDIV: {
      const rv32f_instruction fi{instr};

      FasterFloatType rewritten;
      rewritten.rd = fi.R4type.rd;
      rewritten.rs1 = fi.R4type.rs1;
      rewritten.rs2 = fi.R4type.rs2;
      rewritten.func = fi.R4type.funct2;

      instr.whole = rewritten.whole;
      return bytecode;
    }
    case RV32F_BC_FMADD: {
      // It's unclear how to optimize this instruction
      return bytecode;
    }
    /** Vector instructions **/
    case RV32V_BC_VLE32:
    case RV32V_BC_VSE32: {
      const rv32v_instruction vi{instr};
      FasterMove rewritten;
      rewritten.rd = vi.VLS.vd;
      rewritten.rs1 = vi.VLS.rs1;

      instr.whole = rewritten.whole;
      return bytecode;
    }
    case RV32V_BC_VFADD_VV:
    case RV32V_BC_VFMUL_VF: {
      const rv32v_instruction vi{instr};
      FasterOpType rewritten;
      rewritten.rd = vi.OPVV.vd;
      rewritten.rs1 = vi.OPVV.vs1;
      rewritten.rs2 = vi.OPVV.vs2;

      instr.whole = rewritten.whole;
      return bytecode;
    }
    /** Compressed instructions **/
    case RV32C_BC_FUNCTION: {
      // Already fast, no need to rewrite
      return bytecode;
    }
    case RV32C_BC_ADDI: {
      const rv32c_instruction ci{instr};

      FasterItype rewritten;
      if (ci.opcode() == RISCV_CI_CODE(0b000, 0b00)) {
        // C.ADDI4SPN
        rewritten.rs1 = ci.CIW.srd + 8;
        rewritten.rs2 = REG_SP;
        rewritten.imm = ci.CIW.offset();
      } else if (ci.opcode() == RISCV_CI_CODE(0b011, 0b01)) {
        // C.ADDI16SP
        rewritten.rs1 = REG_SP;
        rewritten.rs2 = REG_SP;
        rewritten.imm = ci.CI16.signed_imm();
      } else { // C.ADDI
        rewritten.rs1 = ci.CI.rd;
        rewritten.rs2 = ci.CI.rd;
        rewritten.imm = ci.CI.signed_imm();
      }

      instr.whole = rewritten.whole;
      return RV32C_BC_ADDI;
    }
    case RV32C_BC_LI: {
      const rv32c_instruction ci{instr};

      FasterItype rewritten;
      rewritten.rs1 = ci.CI.rd;
      rewritten.rs2 = 0;
      rewritten.imm = ci.CI.signed_imm();

      instr.whole = rewritten.whole;
      return RV32C_BC_ADDI;
    }
    case RV32C_BC_MV: {
      const rv32c_instruction ci{instr};

      FasterMove rewritten;
      rewritten.rd = ci.CR.rd;
      rewritten.rs1 = ci.CR.rs2;

      instr.whole = rewritten.whole;
      return RV32C_BC_MV;
    }
    case RV32C_BC_SLLI: {
      const rv32c_instruction ci{instr};

      FasterItype rewritten;
      rewritten.rs1 = ci.CI.rd;
      rewritten.rs2 = 0;
      if constexpr (sizeof(address_t) == 8) rewritten.imm = ci.CI.shift64_imm();
      else rewritten.imm = ci.CI.shift_imm();

      instr.whole = rewritten.whole;
      return RV32C_BC_SLLI;
    }
    case RV32C_BC_SRLI: {
      const rv32c_instruction ci{instr};

      FasterItype rewritten;
      rewritten.rs1 = ci.CA.srd + 8;
      if constexpr (sizeof(address_t) == 8) rewritten.imm = ci.CAB.shift64_imm();
      else rewritten.imm = ci.CAB.shift_imm();

      instr.whole = rewritten.whole;
      return bytecode;
    }
    case RV32C_BC_ANDI: {
      const rv32c_instruction ci{instr};

      FasterItype rewritten;
      rewritten.rs1 = ci.CA.srd + 8;
      rewritten.imm = ci.CAB.signed_imm();

      instr.whole = rewritten.whole;
      return bytecode;
    }
    case RV32C_BC_ADD: {
      const rv32c_instruction ci{instr};

      FasterItype rewritten;
      rewritten.rs1 = ci.CR.rd;
      rewritten.rs2 = ci.CR.rs2;

      instr.whole = rewritten.whole;
      return bytecode;
    }
    case RV32C_BC_XOR:
    case RV32C_BC_OR: {
      const rv32c_instruction ci{instr};

      FasterItype rewritten;
      rewritten.rs1 = ci.CA.srd + 8;
      rewritten.rs2 = ci.CA.srs2 + 8;

      instr.whole = rewritten.whole;
      return bytecode;
    }
    case RV32C_BC_BEQZ:
    case RV32C_BC_BNEZ: {
      const rv32c_instruction ci{instr};

      const int32_t imm = ci.CB.signed_imm();
      const auto addr = pc + imm;

      if (!this->is_within(addr, 2) || (addr % PCAL) != 0) {
        // Allow branch outside of execute segment?
        return RV32I_BC_INVALID; // No, just return invalid
      }

      FasterItype rewritten;
      rewritten.rs1 = ci.CB.srs1 + 8;
      rewritten.rs2 = 0;
      rewritten.imm = imm;

      instr.whole = rewritten.whole;
      return bytecode;
    }
    case RV32C_BC_JMP:
    case RV32C_BC_JAL_ADDIW: {
      const rv32c_instruction ci{instr};

      if (sizeof(address_t) == 8 && bytecode == RV32C_BC_JAL_ADDIW) {
        // C.ADDIW instead
        FasterItype rewritten;
        rewritten.rs1 = ci.CI.rd;
        rewritten.rs2 = ci.CI.rd;
        rewritten.imm = ci.CI.signed_imm();

        instr.whole = rewritten.whole;
        return bytecode;
      }

      const int32_t imm = ci.CJ.signed_imm();
      const auto addr = pc + imm;

      if (!this->is_within(addr, 4) || (addr % PCAL) != 0) {
        return RV32I_BC_INVALID;
      }

      instr.whole = imm;
      return bytecode;
    }
    case RV32C_BC_JALR: {
      const rv32c_instruction ci{instr};
      instr.whole = ci.CR.rd;
      return bytecode;
    }
    case RV32C_BC_JR: {
      const rv32c_instruction ci{instr};
      instr.whole = ci.CR.rd;
      return bytecode;
    }
    case RV32C_BC_LDD: {
      const rv32c_instruction ci{instr};

      FasterItype rewritten;
      if ((ci.opcode() & 0x3) == 0x0) { // C.LD
        rewritten.rs1 = ci.CSD.srs1 + 8;
        rewritten.rs2 = ci.CSD.srs2 + 8;
        rewritten.imm = ci.CSD.offset8();
      } else { // C.LDSP
        rewritten.rs1 = ci.CIFLD.rd;
        rewritten.rs2 = REG_SP;
        rewritten.imm = ci.CIFLD.offset();
      }

      instr.whole = rewritten.whole;
      return bytecode;
    }
    case RV32C_BC_STD: {
      const rv32c_instruction ci{instr};

      FasterItype rewritten;
      if ((ci.opcode() & 0x3) == 0x0) { // C.SD
        rewritten.rs1 = ci.CSD.srs1 + 8;
        rewritten.rs2 = ci.CSD.srs2 + 8;
        rewritten.imm = ci.CSD.offset8();
      } else { // C.SDSP
        rewritten.rs1 = REG_SP;
        rewritten.rs2 = ci.CSFSD.rs2;
        rewritten.imm = ci.CSFSD.offset();
      }

      instr.whole = rewritten.whole;
      return bytecode;
    }
    case RV32C_BC_LDW: {
      const rv32c_instruction ci{instr};

      FasterItype rewritten;
      if ((ci.opcode() & 0x3) == 0x0) { // C.LW
        rewritten.rs1 = ci.CL.srd + 8;
        rewritten.rs2 = ci.CL.srs1 + 8;
        rewritten.imm = ci.CL.offset();
      } else { // C.LWSP
        rewritten.rs1 = ci.CI2.rd;
        rewritten.rs2 = REG_SP;
        rewritten.imm = ci.CI2.offset();
      }

      instr.whole = rewritten.whole;
      return bytecode;
    }
    case RV32C_BC_STW: {
      const rv32c_instruction ci{instr};

      FasterItype rewritten;
      if ((ci.opcode() & 0x3) == 0x0) { // C.SW
        rewritten.rs1 = ci.CS.srs1 + 8;
        rewritten.rs2 = ci.CS.srs2 + 8;
        rewritten.imm = ci.CS.offset4();
      } else { // C.SWSP
        rewritten.rs1 = REG_SP;
        rewritten.rs2 = ci.CSS.rs2;
        rewritten.imm = ci.CSS.offset(4);
      }

      instr.whole = rewritten.whole;
      return bytecode;
    }

    case RV32I_BC_SYSCALL: {
      return RV32I_BC_SYSCALL;
    }
    case RV32I_BC_LIVEPATCH: {
      throw std::runtime_error("Live-patch bytecode is not valid here");
    }
    default: throw std::runtime_error("Invalid bytecode " + std::to_string(bytecode) + " for threaded rewrite");
    }

    return bytecode;
  }
} // riscv
