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
#include <cstdio>
#include "../../notraced_riscv_isa3_system.hpp"
#include "./sysprint.hpp"

namespace riscv
{
	template <AddressType address_t>
	static void syscall_stub_zero(Machine<address_t>& machine)
	{
		SYSPRINT("SYSCALL stubbed (zero): %d\n", (int)machine.cpu.reg(17));
		machine.set_result(0);
	}

	template <AddressType address_t>
	static void syscall_stub_nosys(Machine<address_t>& machine)
	{
		SYSPRINT("SYSCALL stubbed (nosys): %d\n", (int)machine.cpu.reg(17));
		machine.set_result(-38); // ENOSYS
	}

	template <AddressType address_t>
	static void syscall_ebreak(riscv::Machine<address_t>& machine)
	{
		printf("\n>>> EBREAK at %#lX\n", (long)machine.cpu.pc());
		throw MachineException(UNHANDLED_SYSCALL, "EBREAK instruction");
	}

	template<AddressType address_t>
	static void syscall_write(Machine<address_t>& machine)
	{
		const int vfd = machine.template sysarg<int>(0);
		const auto address = machine.sysarg(1);
		const size_t len = machine.sysarg(2);
		SYSPRINT("SYSCALL write, fd: %d addr: 0x%lX, len: %zu\n",
				vfd, (long) address, len);
		// We only accept standard output pipes, for now :)
		if (vfd == 1 || vfd == 2) {
			// Zero-copy retrieval of buffers (64kb)
			riscv::vBuffer buffers[16];
			const size_t cnt =
				machine.memory.gather_buffers_from_range(16, buffers, address, len);
			for (size_t i = 0; i < cnt; i++) {
				machine.print(buffers[i].ptr, buffers[i].len);
			}
			machine.set_result(len);
			return;
		}
		machine.set_result(-EBADF);
	}

	template <AddressType address_t>
	static void syscall_exit(Machine<address_t>& machine)
	{
		// Stop sets the max instruction counter to zero, allowing most
		// instruction loops to end. It is, however, not the only way
		// to exit a program. Tighter integrations with the library should
		// provide their own methods.
		machine.stop();
	}

	template <AddressType address_t>
	static void syscall_brk(Machine<address_t>& machine)
	{
		auto new_end = machine.sysarg(0);
		if (new_end > machine.memory.heap_address() + Memory<address_t>::BRK_MAX) {
			new_end = machine.memory.heap_address() + Memory<address_t>::BRK_MAX;
		} else if (new_end < machine.memory.heap_address()) {
			new_end = machine.memory.heap_address();
		}

		SYSPRINT("SYSCALL brk, new_end: 0x%lX\n", (long)new_end);
		machine.set_result(new_end);
	}

	template <AddressType address_t>
	void Machine<address_t>::setup_minimal_syscalls()
	{
		install_syscall_handler(SYSCALL_EBREAK, syscall_ebreak<address_t>);
		install_syscall_handler(57, syscall_stub_zero<address_t>);  // close
		install_syscall_handler(62, syscall_stub_nosys<address_t>); // lseek
		install_syscall_handler(64, syscall_write<address_t>);
		install_syscall_handler(80, syscall_stub_nosys<address_t>); // fstat
		install_syscall_handler(93, syscall_exit<address_t>);
		install_syscall_handler(214, syscall_brk<address_t>);
	}

  template void Machine<uint32_t>::setup_minimal_syscalls();
  template void Machine<uint64_t>::setup_minimal_syscalls();
} // riscv
