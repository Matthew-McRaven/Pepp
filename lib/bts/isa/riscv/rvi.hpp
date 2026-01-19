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
#include "bts/isa/riscv/rv_types.hpp"

namespace riscv
{
	union rv32i_instruction
	{
		// register format
		struct {
			uint32_t opcode : 7;
			uint32_t rd     : 5;
			uint32_t funct3 : 3;
			uint32_t rs1    : 5;
			uint32_t rs2    : 5;
			uint32_t funct7 : 7;

			bool is_f7() const noexcept {
				return funct7 == 0b0100000;
			}
			bool is_32M() const noexcept {
				return funct7 == 0b0000001;
			}
			uint32_t jumptable_friendly_op() const noexcept {
				// use bit 4 for RV32M extension
				return funct3 | (funct7 << 4);
			}
		} Rtype;
		// immediate format
		struct {
			uint32_t opcode : 7;
			uint32_t rd     : 5;
			uint32_t funct3 : 3;
			uint32_t rs1    : 5;
			uint32_t imm    : 12;

			bool sign() const noexcept {
				return imm & 0x800;
			}
			int32_t signed_imm() const noexcept {
				// Arithmetic shift for sign-extension
				return int32_t(imm << 20) >> 20;
			}
			uint32_t shift_imm() const noexcept {
				return imm & 0x1F;
			}
			uint32_t shift64_imm() const noexcept {
				return imm & 0x3F;
			}
			uint32_t shift128_imm() const noexcept {
				return imm & 0x7F;
			}

      template <AddressType address_t> auto is_rev8() const noexcept {
        if constexpr (sizeof(address_t) == 8) return imm == 0b011010111000;
        else return imm == 0b011010011000;
      }
      auto high_bits() const noexcept { return imm & 0xFC0; }
      bool is_srai() const noexcept { return (imm & 0xFC0) == 0x400; }
      bool is_rori() const noexcept {
				return (imm & 0xFC0) == 0x600;
			}
		} Itype;
		// store format
		struct {
			uint32_t opcode : 7;
			uint32_t imm1   : 5;
			uint32_t funct3 : 3;
			uint32_t rs1    : 5;
			uint32_t rs2    : 5;
			uint32_t imm2   : 7;

			bool sign() const noexcept {
				return imm2 & 0x40;
			}
			int32_t signed_imm() const noexcept {
				const int32_t imm = imm1 | (imm2 << 5);
				return (imm << 20) >> 20;
			}
		} Stype;
		// upper immediate format
		struct {
			uint32_t opcode : 7;
			uint32_t rd     : 5;
			uint32_t imm    : 20;

			int32_t upper_imm() const noexcept {
				return imm << 12u;
			}
		} Utype;
		// branch type
		struct {
			uint32_t opcode : 7;
			uint32_t imm1   : 1;
			uint32_t imm2   : 4;
			uint32_t funct3 : 3;
			uint32_t rs1    : 5;
			uint32_t rs2    : 5;
			uint32_t imm3   : 6;
			uint32_t imm4   : 1;

			bool sign() const noexcept {
				return imm4;
			}
			int32_t signed_imm() const noexcept {
				constexpr uint32_t ext = 0xFFFFF000;
				return (imm2 << 1) | (imm3 << 5) | (imm1 << 11) | (sign() ? ext : 0);
			}
		} Btype;
		// jump instructions
		struct {
			uint32_t opcode : 7;
			uint32_t rd     : 5;
			uint32_t imm1   : 8;
			uint32_t imm2   : 1;
			uint32_t imm3   : 10;
			uint32_t imm4   : 1;

			bool sign() const noexcept {
				return imm4;
			}
			int32_t jump_offset() const noexcept {
				constexpr uint32_t ext = 0xFFF00000;
				const int32_t jo  = (imm3 << 1) | (imm2 << 11) | (imm1 << 12);
				return jo | (sign() ? ext : 0);
			}
		} Jtype;
		// atomic format
		struct {
			uint32_t opcode : 7;
			uint32_t rd     : 5;
			uint32_t funct3 : 3;
			uint32_t rs1    : 5;
			uint32_t rs2    : 5;
			uint32_t rl     : 1;
			uint32_t aq     : 1;
			uint32_t funct5 : 5;
		} Atype;

		uint8_t bytes[4];
		uint16_t half[2];
		uint32_t whole;

		constexpr rv32i_instruction() : whole(0) {}
		constexpr rv32i_instruction(uint32_t another) : whole(another) {}

		uint32_t opcode() const noexcept {
			return Rtype.opcode;
		}
		bool is_illegal() const noexcept {
			return half[0] == 0x0000;
		}

		bool is_long() const noexcept {
			return (whole & 0x3) == 0x3;
		}
		bool is_compressed() const noexcept {
			return (whole & 0x3) != 0x3;
		}
		uint32_t length() const noexcept {
			//return is_long() ? 4 : 2;
			return 2 + 2 * is_long();
		}

		inline uint32_t fpfunc() const noexcept {
			return whole >> (32 - 5);
		}

		inline uint32_t vwidth() const noexcept {
			return (whole >> 12) & 0x7;
		}
		inline uint32_t vsetfunc() const noexcept {
			return whole >> 30;
		}
	};
	static_assert(sizeof(rv32i_instruction) == 4, "Instruction is 4 bytes");
} // riscv
