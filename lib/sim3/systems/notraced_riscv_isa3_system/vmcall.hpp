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
#include "../notraced_riscv_isa3_system.hpp"

namespace riscv {
template <AddressType address_t> template <typename... Args> constexpr inline void Machine<address_t>::setup_call(Args &&...args) {
  cpu.reg(REG_RA) = memory.exit_address();
  [[maybe_unused]] int iarg = REG_ARG0;
	[[maybe_unused]] int farg = REG_FA0;
	([&] {
		if constexpr (std::is_integral_v<remove_cvref<Args>>) {
			cpu.reg(iarg++) = args;
      if constexpr (sizeof(Args) > sizeof(address_t)) // upper 32-bits for 64-bit integers
        cpu.reg(iarg++) = args >> 32;
    }
		else if constexpr (is_stdstring<remove_cvref<Args>>::value)
			cpu.reg(iarg++) = stack_push(args.data(), args.size()+1);
		else if constexpr (is_string<Args>::value)
			cpu.reg(iarg++) = stack_push(args, strlen(args)+1);
#ifdef __cpp_exceptions
		else if constexpr (std::is_same_v<GuestStdString<address_t>, remove_cvref<Args>>) {
			args.move(cpu.reg(REG_SP) - sizeof(Args)); // SSO-adjustment
			cpu.reg(iarg++) = stack_push(&args, sizeof(Args));
    } else if constexpr (is_scoped_guest_object<address_t, remove_cvref<Args>>::value) {
      cpu.reg(iarg++) = args.address();
    }
#endif
    else if constexpr (is_stdvector<remove_cvref<Args>>::value)
      cpu.reg(iarg++) = stack_push(args.data(), args.size() * sizeof(args[0]));
		else if constexpr (std::is_same_v<float, remove_cvref<Args>>)
			cpu.registers().getfl(farg++).set_float(args);
		else if constexpr (std::is_same_v<double, remove_cvref<Args>>)
			cpu.registers().getfl(farg++).f64 = args;
		else if constexpr (std::is_enum_v<remove_cvref<Args>>)
			cpu.reg(iarg++) = int(args);
		else if constexpr (std::is_standard_layout_v<remove_cvref<Args>>)
			cpu.reg(iarg++) = stack_push(&args, sizeof(args));
		else
			static_assert(always_false<decltype(args)>, "Unknown type");
	}(), ...);
	cpu.reg(REG_SP) &= ~address_t(0xF);
}

template <AddressType address_t>
template <uint64_t MAXI, bool Throw, typename... Args> constexpr
inline address_t Machine<address_t>::vmcall(address_t pc, Args&&... args)
{
	// reset the stack pointer to an initial location (deliberately)
	this->cpu.reset_stack_pointer();
	// setup calling convention
	this->setup_call(std::forward<Args>(args)...);
	// execute guest function
  if constexpr (MAXI == UINT64_MAX || MAXI == 0u) {
    this->simulate_with<Throw>(MAXI, 0u, pc);
  } else {
    this->simulate_with<Throw>(MAXI, 0u, pc);
  }

  // address-sized integer return value
	return cpu.reg(REG_ARG0);
}

template <AddressType address_t>
template <uint64_t MAXI, bool Throw, typename... Args> constexpr
inline address_t Machine<address_t>::vmcall(const char* funcname, Args&&... args)
{
	address_t call_addr = memory.resolve_address(funcname);
	return vmcall<MAXI, Throw>(call_addr, std::forward<Args>(args)...);
}

template <AddressType address_t>
template <bool Throw, bool StoreRegs, typename... Args> inline
address_t Machine<address_t>::preempt(uint64_t max_instr, address_t call_addr, Args&&... args)
{
	Registers<address_t> regs;
	if constexpr (StoreRegs) {
		regs = cpu.registers();
	}
	// we need to make some stack room
	this->cpu.reg(REG_SP) -= 16u;
	// setup calling convention
	this->setup_call(std::forward<Args>(args)...);
	// execute!
	return this->cpu.preempt_internal(regs, Throw, StoreRegs, call_addr, max_instr);
}

template <AddressType address_t>
template <bool Throw, bool StoreRegs, typename... Args> inline
address_t Machine<address_t>::preempt(uint64_t max_instr, const char* funcname, Args&&... args)
{
	address_t call_addr = memory.resolve_address(funcname);
	return preempt<Throw, StoreRegs>(max_instr, call_addr, std::forward<Args>(args)...);
}

} // namespace riscv
