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
#include "core/arch/riscv/isa/rv_base.hpp"
#include "core/arch/riscv/isa/rv_types.hpp"
#include "core/arch/riscv/isa/rvc.hpp"
#include "core/arch/riscv/isa/rvfd.hpp"
#include "core/arch/riscv/isa/rvi.hpp"
#include "core/arch/riscv/isa/rvv.hpp"
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

		size_t threaded_rewrite(size_t bytecode, address_t pc, instruction_format& instr);

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
                                                                   instruction_format &instr) {
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
      const auto u = original.as<InstructionU>();
      rewritten.rd = u.rd;
      rewritten.offset = u.imm << 4;

      instr.whole = rewritten.whole;
      return bytecode;
    }
    case RV32I_BC_MV: {
      FasterMove rewritten;
      const auto i = original.as<InstructionI>();
      rewritten.rd = i.rd;
      rewritten.rs1 = i.rs1;

      instr.whole = rewritten.whole;
      return bytecode;
    }
    case RV32I_BC_LI: {
      FasterImmediate rewritten;
      const auto i = original.as<InstructionI>();
      rewritten.rd = i.rd;
      rewritten.imm = i.signed_imm();

      instr.whole = rewritten.whole;
      return bytecode;
    }
    case RV64I_BC_SLLIW:
    case RV64I_BC_SRLIW:
    case RV64I_BC_SRAIW: {
      if (sizeof(address_t) == 4) return RV32I_BC_INVALID;

      FasterItype rewritten;
      const auto i = original.as<InstructionI>();
      rewritten.rs1 = i.rd;
      rewritten.rs2 = i.rs1;
      rewritten.imm = i.imm & 31;

      instr.whole = rewritten.whole;
      return bytecode;
    }
    case RV32I_BC_SLLI:
    case RV32I_BC_SRLI:
    case RV32I_BC_SRAI:
    case RV32I_BC_BSETI:
    case RV32I_BC_BEXTI: {
      FasterItype rewritten;
      const auto i = original.as<InstructionI>();
      rewritten.rs1 = i.rd;
      rewritten.rs2 = i.rs1;
      rewritten.imm = i.imm & (XLEN - 1);

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
      const auto i = original.as<InstructionI>();
      rewritten.rs1 = i.rd;
      rewritten.rs2 = i.rs1;
      rewritten.imm = i.signed_imm();

      instr.whole = rewritten.whole;
      return bytecode;
    }
    case RV32I_BC_BEQ:
    case RV32I_BC_BNE:
    case RV32I_BC_BLT:
    case RV32I_BC_BGE:
    case RV32I_BC_BLTU:
    case RV32I_BC_BGEU: {
      const auto b = original.as<InstructionB>();
      const int32_t imm = b.signed_imm();
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
      rewritten.rs1 = b.rs1;
      rewritten.rs2 = b.rs2;
      rewritten.imm = b.signed_imm();

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
      const auto rt = original.as<InstructionR>();
      rewritten.rd = rt.rd;
      rewritten.rs1 = rt.rs1;
      rewritten.rs2 = rt.rs2;

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
      const auto i = original.as<InstructionI>();
      rewritten.rs1 = i.rd;
      rewritten.rs2 = i.rs1;
      rewritten.imm = i.signed_imm();

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
      const auto st = original.as<InstructionS>();
      rewritten.rs1 = st.rs1;
      rewritten.rs2 = st.rs2;
      rewritten.imm = st.signed_imm();

      instr.whole = rewritten.whole;
      return bytecode;
    }
    case RV32F_BC_FLW:
    case RV32F_BC_FLD: {
      const auto i = original.as<InstructionI>();
      FasterItype rewritten;
      rewritten.rs1 = i.rd;
      rewritten.rs2 = i.rs1;
      rewritten.imm = i.signed_imm();

      instr.whole = rewritten.whole;
      return bytecode;
    }
    case RV32F_BC_FSW:
    case RV32F_BC_FSD: {
      const auto st = original.as<InstructionS>();
      FasterItype rewritten;
      rewritten.rs1 = st.rs1;
      rewritten.rs2 = st.rs2;
      rewritten.imm = st.signed_imm();

      instr.whole = rewritten.whole;
      return bytecode;
    }
    case RV32I_BC_JAL: {
      const auto j = original.as<InstructionJ>();
      const auto addr = pc + j.jump_offset();
      const bool is_aligned = addr % PCAL == 0;
      const bool store_zero = j.rd == 0;
      const bool store_ra = j.rd == REG_RA;

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
        rewritten.offset = j.jump_offset();
        rewritten.rd = j.rd;

        instr.whole = rewritten.whole;
        return bytecode;
      }

      return RV32I_BC_INVALID;
    }
    case RV32I_BC_JALR: {
      FasterItype rewritten;
      const auto i = original.as<InstructionI>();
      rewritten.imm = i.signed_imm();
      rewritten.rs1 = i.rd;
      rewritten.rs2 = i.rs1;

      instr.whole = rewritten.whole;
      return bytecode;
    }
    /** FP 32- and 64-bit instructions **/
    case RV32F_BC_FADD:
    case RV32F_BC_FSUB:
    case RV32F_BC_FMUL:
    case RV32F_BC_FDIV: {
      const auto f = instr.as<InstructionRFP>();

      FasterFloatType rewritten;
      rewritten.rd = f.rd;
      rewritten.rs1 = f.rs1;
      rewritten.rs2 = f.rs2;
      rewritten.func = f.fmt;

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
      const auto vls = instr.as<InstructionVLS>();
      FasterMove rewritten;
      rewritten.rd = vls.vd;
      rewritten.rs1 = vls.rs1;

      instr.whole = rewritten.whole;
      return bytecode;
    }
    case RV32V_BC_VFADD_VV:
    case RV32V_BC_VFMUL_VF: {
      const auto opvv = instr.as<InstructionOPVV>();
      FasterOpType rewritten;
      rewritten.rd = opvv.vd;
      rewritten.rs1 = opvv.vs1;
      rewritten.rs2 = opvv.vs2;

      instr.whole = rewritten.whole;
      return bytecode;
    }
    /** Compressed instructions **/
    case RV32C_BC_FUNCTION: {
      // Already fast, no need to rewrite
      return bytecode;
    }
    case RV32C_BC_ADDI: {
      const auto ciw = instr.as_compressed<InstructionCIW>();
      const auto ci16 = instr.as_compressed<InstructionCI16>();
      const auto ci = instr.as_compressed<InstructionCI>();

      FasterItype rewritten;
      if (instr.copcode() == ((0b000 << 13) | 0b00)) {
        // C.ADDI4SPN
        rewritten.rs1 = ciw.srd + 8;
        rewritten.rs2 = REG_SP;
        rewritten.imm = ciw.offset();
      } else if (instr.copcode() == ((0b011 << 13) | 0b01)) {
        // C.ADDI16SP
        rewritten.rs1 = REG_SP;
        rewritten.rs2 = REG_SP;
        rewritten.imm = ci16.signed_imm();
      } else { // C.ADDI
        rewritten.rs1 = ci.rd;
        rewritten.rs2 = ci.rd;
        rewritten.imm = ci.signed_imm();
      }

      instr.whole = rewritten.whole;
      return RV32C_BC_ADDI;
    }
    case RV32C_BC_LI: {
      const auto ci = instr.as_compressed<InstructionCI>();

      FasterItype rewritten;
      rewritten.rs1 = ci.rd;
      rewritten.rs2 = 0;
      rewritten.imm = ci.signed_imm();

      instr.whole = rewritten.whole;
      return RV32C_BC_ADDI;
    }
    case RV32C_BC_MV: {
      const auto cr = instr.as_compressed<InstructionCR>();

      FasterMove rewritten;
      rewritten.rd = cr.rd;
      rewritten.rs1 = cr.rs2;

      instr.whole = rewritten.whole;
      return RV32C_BC_MV;
    }
    case RV32C_BC_SLLI: {
      const auto ci = instr.as_compressed<InstructionCI>();

      FasterItype rewritten;
      rewritten.rs1 = ci.rd;
      rewritten.rs2 = 0;
      if constexpr (sizeof(address_t) == 8) rewritten.imm = ci.shift64_imm();
      else rewritten.imm = ci.shift_imm();

      instr.whole = rewritten.whole;
      return RV32C_BC_SLLI;
    }
    case RV32C_BC_SRLI: {
      const auto ca = instr.as_compressed<InstructionCA>();
      const auto cab = instr.as_compressed<InstructionCAB>();

      FasterItype rewritten;
      rewritten.rs1 = ca.srd + 8;
      if constexpr (sizeof(address_t) == 8) rewritten.imm = cab.shift64_imm();
      else rewritten.imm = cab.shift_imm();

      instr.whole = rewritten.whole;
      return bytecode;
    }
    case RV32C_BC_ANDI: {
      const auto ca = instr.as_compressed<InstructionCA>();
      const auto cab = instr.as_compressed<InstructionCAB>();

      FasterItype rewritten;
      rewritten.rs1 = ca.srd + 8;
      rewritten.imm = cab.signed_imm();

      instr.whole = rewritten.whole;
      return bytecode;
    }
    case RV32C_BC_ADD: {
      const auto cr = instr.as_compressed<InstructionCR>();

      FasterItype rewritten;
      rewritten.rs1 = cr.rd;
      rewritten.rs2 = cr.rs2;

      instr.whole = rewritten.whole;
      return bytecode;
    }
    case RV32C_BC_XOR:
    case RV32C_BC_OR: {
      const auto ca = instr.as_compressed<InstructionCA>();

      FasterItype rewritten;
      rewritten.rs1 = ca.srd + 8;
      rewritten.rs2 = ca.srs2 + 8;

      instr.whole = rewritten.whole;
      return bytecode;
    }
    case RV32C_BC_BEQZ:
    case RV32C_BC_BNEZ: {
      const auto cb = instr.as_compressed<InstructionCB>();

      const int32_t imm = cb.signed_imm();
      const auto addr = pc + imm;

      if (!this->is_within(addr, 2) || (addr % PCAL) != 0) {
        // Allow branch outside of execute segment?
        return RV32I_BC_INVALID; // No, just return invalid
      }

      FasterItype rewritten;
      rewritten.rs1 = cb.srs1 + 8;
      rewritten.rs2 = 0;
      rewritten.imm = imm;

      instr.whole = rewritten.whole;
      return bytecode;
    }
    case RV32C_BC_JMP:
    case RV32C_BC_JAL_ADDIW: {
      const auto ci = instr.as_compressed<InstructionCI>();
      const auto cj = instr.as_compressed<InstructionCJ>();

      if (sizeof(address_t) == 8 && bytecode == RV32C_BC_JAL_ADDIW) {
        // C.ADDIW instead
        FasterItype rewritten;
        rewritten.rs1 = ci.rd;
        rewritten.rs2 = ci.rd;
        rewritten.imm = ci.signed_imm();

        instr.whole = rewritten.whole;
        return bytecode;
      }

      const int32_t imm = cj.signed_imm();
      const auto addr = pc + imm;

      if (!this->is_within(addr, 4) || (addr % PCAL) != 0) {
        return RV32I_BC_INVALID;
      }

      instr.whole = imm;
      return bytecode;
    }
    case RV32C_BC_JALR: {
      const auto cr = instr.as_compressed<InstructionCR>();
      instr.whole = cr.rd;
      return bytecode;
    }
    case RV32C_BC_JR: {
      const auto cr = instr.as_compressed<InstructionCR>();
      instr.whole = cr.rd;
      return bytecode;
    }
    case RV32C_BC_LDD: {
      const auto csd = instr.as_compressed<InstructionCSD>();
      const auto cifld = instr.as_compressed<InstructionCIFLD>();

      FasterItype rewritten;
      if ((instr.copcode() & 0x3) == 0x0) { // C.LD
        rewritten.rs1 = csd.srs1 + 8;
        rewritten.rs2 = csd.srs2 + 8;
        rewritten.imm = csd.offset8();
      } else { // C.LDSP
        rewritten.rs1 = cifld.rd;
        rewritten.rs2 = REG_SP;
        rewritten.imm = cifld.offset();
      }

      instr.whole = rewritten.whole;
      return bytecode;
    }
    case RV32C_BC_STD: {
      const auto csd = instr.as_compressed<InstructionCSD>();
      const auto csfsd = instr.as_compressed<InstructionCSFSD>();

      FasterItype rewritten;
      if ((instr.copcode() & 0x3) == 0x0) { // C.SD
        rewritten.rs1 = csd.srs1 + 8;
        rewritten.rs2 = csd.srs2 + 8;
        rewritten.imm = csd.offset8();
      } else { // C.SDSP
        rewritten.rs1 = REG_SP;
        rewritten.rs2 = csfsd.rs2;
        rewritten.imm = csfsd.offset();
      }

      instr.whole = rewritten.whole;
      return bytecode;
    }
    case RV32C_BC_LDW: {
      const auto cl = instr.as_compressed<InstructionCL>();
      const auto ci2 = instr.as_compressed<InstructionCI2>();

      FasterItype rewritten;
      if ((instr.copcode() & 0x3) == 0x0) { // C.LW
        rewritten.rs1 = cl.srd + 8;
        rewritten.rs2 = cl.srs1 + 8;
        rewritten.imm = cl.offset();
      } else { // C.LWSP
        rewritten.rs1 = ci2.rd;
        rewritten.rs2 = REG_SP;
        rewritten.imm = ci2.offset();
      }

      instr.whole = rewritten.whole;
      return bytecode;
    }
    case RV32C_BC_STW: {
      const auto cs = instr.as_compressed<InstructionCS>();
      const auto css = instr.as_compressed<InstructionCSS>();

      FasterItype rewritten;
      if ((instr.copcode() & 0x3) == 0x0) { // C.SW
        rewritten.rs1 = cs.srs1 + 8;
        rewritten.rs2 = cs.srs2 + 8;
        rewritten.imm = cs.offset4();
      } else { // C.SWSP
        rewritten.rs1 = REG_SP;
        rewritten.rs2 = css.rs2;
        rewritten.imm = css.offset(4);
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
