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
#include <functional>
#include <unordered_map>
#include "core/arch/riscv/isa/rv_types.hpp"
#include "sim3/systems/notraced_riscv_isa3_system.hpp"

namespace riscv
{
	template <AddressType address_t>
	struct DebugMachine
	{
		using breakpoint_t = std::function<void(DebugMachine<address_t> &)>;
		using printer_func_t = void(*)(const Machine<address_t>&, const char*, size_t);
		struct Watchpoint {
			address_t addr;
			size_t    len;
			address_t last_value;
			breakpoint_t callback;
		};

		void simulate(uint64_t max = UINT64_MAX);
		void simulate(breakpoint_t callback, uint64_t max = UINT64_MAX);
		void print(const std::string& label = "Breakpoint", address_t pc = 0);
		void print_and_pause();

		// Immediately block execution, print registers and current instruction.
		bool verbose_instructions = false;
		bool verbose_jumps = false;
		bool verbose_registers = false;
		bool verbose_fp_registers = false;

		void breakpoint(address_t address, breakpoint_t = default_pausepoint);
		void erase_breakpoint(address_t address) { breakpoint(address, nullptr); }
		auto& breakpoints() { return this->m_breakpoints; }
		void break_on_steps(int steps);
		void break_checks();
		static void default_pausepoint(DebugMachine<address_t>&);

		void watchpoint(address_t address, size_t len, breakpoint_t = default_pausepoint);
		void erase_watchpoint(address_t address) { watchpoint(address, 0, nullptr); }

		// Debug printer (for printing exceptions)
		void debug_print(const char*, size_t) const;
		auto& get_debug_printer() const noexcept { return m_debug_printer; }
		void set_debug_printer(printer_func_t pf) noexcept { m_debug_printer = pf; }

		Machine<address_t>& machine;
		DebugMachine(Machine<address_t>& m);
	private:
		void print_help() const;
		void dprintf(const char* fmt, ...) const;
		bool execute_commands();
		// instruction step & breakpoints
		mutable int32_t m_break_steps = 0;
		mutable int32_t m_break_steps_cnt = 0;
		mutable printer_func_t m_debug_printer = nullptr;
		std::unordered_map<address_t, breakpoint_t> m_breakpoints;
		std::vector<Watchpoint> m_watchpoints;
		bool break_time() const;
		void register_debug_logging() const;
	};

	template <AddressType address_t>
	inline void DebugMachine<address_t>::breakpoint(address_t addr, breakpoint_t func)
	{
		if (func)
			this->m_breakpoints[addr] = func;
		else
			this->m_breakpoints.erase(addr);
	}

	template <AddressType address_t>
	inline void DebugMachine<address_t>::watchpoint(address_t addr, size_t len, breakpoint_t func)
	{
		if (func) {
			this->m_watchpoints.push_back(Watchpoint{
				.addr = addr,
				.len  = len,
				.last_value = 0,
				.callback = func,
			});
		} else {
			for (auto it = m_watchpoints.begin(); it != m_watchpoints.end();) {
				if (it->addr == addr) {
					m_watchpoints.erase(it);
					return;
				} else ++it;
			}
		}
	}

	template <AddressType address_t>
	inline void DebugMachine<address_t>::default_pausepoint(DebugMachine& debug)
	{
		debug.print_and_pause();
	}

} // riscv
