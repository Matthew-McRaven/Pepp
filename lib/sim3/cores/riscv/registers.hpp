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
#include <array>
#include <memory>
#include <string>
#include "./rvv_registers.hpp"
#include "enums/isa/rv_types.hpp"
#include "sim3/common_macros.hpp"
#include "sim3/systems/notraced_riscv_isa3_system/rv_common.hpp"

namespace riscv
{
	union fp64reg {
		int32_t i32[2];
		float   f32[2];
		int64_t i64 = 0;
		double  f64;
		struct {
			uint32_t bits  : 31;
			uint32_t sign  : 1;
			uint32_t upper;
		} lsign;
		struct {
			uint64_t bits  : 63;
			uint64_t sign  : 1;
		} usign;

		inline void nanbox() {
			if constexpr (fcsr_emulation)
				this->i32[1] = ~0;
			else
				this->i32[1] = 0;
		}
		void set_float(float f) {
			this->f32[0] = f;
			if constexpr (nanboxing)
				this->nanbox();
		}
		void set_double(double d) {
			this->f64 = d;
		}
		void load_u32(uint32_t val) {
			this->i32[0] = val;
			if constexpr (nanboxing)
				this->nanbox();
		}
		void load_u64(uint64_t val) {
			this->i64 = val;
		}

		template <typename T>
		T get() const {
			if constexpr (sizeof(T) == 4) {
				return static_cast<T>(this->f32[0]);
			} else {
				return static_cast<T>(this->f64);
			}
		}
		template <typename T>
		void set(T val) {
			if constexpr (sizeof(T) == 4) {
				this->f32[0] = static_cast<float>(val);
			} else {
				this->f64 = static_cast<double>(val);
			}
		}
	};

  template <AddressType address_t> struct alignas(RISCV_MACHINE_ALIGNMENT) Registers {
    using register_t = register_type<address_t>; // integer register
    union FCSR {
      uint32_t whole = 0;
			struct {
				uint32_t fflags : 5;
				uint32_t frm    : 3;
				uint32_t resv24 : 24;
			};
    };

    PEPP_ALWAYS_INLINE auto& get() noexcept { return m_reg; }
		PEPP_ALWAYS_INLINE const auto& get() const noexcept { return m_reg; }

		PEPP_ALWAYS_INLINE register_t& get(uint32_t idx) noexcept { return m_reg[idx]; }
		PEPP_ALWAYS_INLINE const register_t& get(uint32_t idx) const noexcept { return m_reg[idx]; }

		PEPP_ALWAYS_INLINE fp64reg& getfl(uint32_t idx) noexcept { return m_regfl[idx]; }
		PEPP_ALWAYS_INLINE const fp64reg& getfl(uint32_t idx) const noexcept { return m_regfl[idx]; }

		register_t& at(uint32_t idx) { return m_reg.at(idx); }
		const register_t& at(uint32_t idx) const { return m_reg.at(idx); }

		FCSR& fcsr() noexcept { return m_fcsr; }

		std::string to_string() const;
		std::string flp_to_string() const;

		auto& rvv() noexcept {
			return m_rvv;
		}
		const auto& rvv() const noexcept {
			return m_rvv;
		}
		bool has_vectors() const noexcept { return vector_extension; }

		Registers() = default;
		Registers(const Registers& other)
			: m_reg { other.m_reg }, pc { other.pc }, m_fcsr { other.m_fcsr }, m_regfl { other.m_regfl }
		{
			m_rvv = other.m_rvv;
		}
		enum class Options { Everything, NoVectors };

		Registers& operator =(const Registers& other) {
			this->copy_from(Options::Everything, other);
			return *this;
		}
		inline void copy_from(Options opts, const Registers& other) {
			this->pc    = other.pc;
			this->m_reg = other.m_reg;
			this->m_fcsr = other.m_fcsr;
			this->m_regfl = other.m_regfl;
			if (opts == Options::Everything) {
				m_rvv = other.m_rvv;
			}
			(void)opts;
		}

	private:
		// General purpose registers
		std::array<register_t, 32> m_reg {};
	public:
		address_t pc = 0;
	private:
		// FP control register
		FCSR m_fcsr {};
		// General FP registers
		std::array<fp64reg, 32> m_regfl {};
    VectorRegisters<address_t> m_rvv;
  };

  static_assert(sizeof(fp64reg) == 8, "FP-register is 64-bit");
} // riscv
