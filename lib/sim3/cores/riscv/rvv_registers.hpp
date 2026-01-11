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
#include "bts/isa/rv_types.hpp"
#include "sim3/common_macros.hpp"
#include "sim3/systems/notraced_riscv_isa3_system/rv_common.hpp"

namespace riscv
{
	union alignas(RISCV_EXT_VECTOR) VectorLane {
		static constexpr unsigned VSIZE = RISCV_EXT_VECTOR;
		static constexpr unsigned size() noexcept { return VSIZE; }

		std::array<uint8_t,  VSIZE / 1> u8 = {};
		std::array<uint16_t, VSIZE / 2> u16;
		std::array<uint32_t, VSIZE / 4> u32;
		std::array<uint64_t, VSIZE / 8> u64;

		std::array<float,  VSIZE / 4> f32;
		std::array<double, VSIZE / 8> f64;
	};
	static_assert(sizeof(VectorLane) == RISCV_EXT_VECTOR, "Vectors are 32 bytes");
	static_assert(alignof(VectorLane) == RISCV_EXT_VECTOR, "Vectors are 32-byte aligned");

  template <AddressType address_t> struct alignas(RISCV_EXT_VECTOR) VectorRegisters {
    using register_t = register_type<address_t>; // integer register

    auto& get(unsigned idx) noexcept { return m_vec[idx]; }
		auto& f32(unsigned idx) { return m_vec[idx].f32; }
		auto& u32(unsigned idx) { return m_vec[idx].u32; }

		register_t vtype() const noexcept {
			return 0u;
		}


	private:
		std::array<VectorLane, 32> m_vec {};
  };
}
