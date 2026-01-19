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

#include "../sysprint.hpp"
#include "bts/isa/riscv/rv_types.hpp"
#include "sim3/systems/notraced_riscv_isa3_system.hpp"
namespace riscv {
template <AddressType address_t>
static void syscall_eventfd2(Machine<address_t>& machine)
{
  [[maybe_unused]] const auto initval = machine.template sysarg<int>(0);
  [[maybe_unused]] const auto flags = machine.template sysarg<int>(1);
  [[maybe_unused]] int real_fd = -1;

  if (machine.has_file_descriptors()) {
    machine.set_result(123456780);
  } else {
    machine.set_result(-1);
  }
  SYSPRINT("SYSCALL eventfd2(initval: %X flags: %#x real_fd: %d) = %d\n", initval, flags, real_fd,
           machine.template return_value<int>());
}

template <AddressType address_t>
static void syscall_epoll_create(Machine<address_t>& machine)
{
  [[maybe_unused]] const auto flags = machine.template sysarg<int>(0);
  [[maybe_unused]] int real_fd = -1;

  if (machine.has_file_descriptors()) {
    machine.set_result(123456781);
  } else {
    machine.set_result(-1);
  }
  SYSPRINT("SYSCALL epoll_create(real_fd: %d), flags: %#x = %d\n", real_fd, flags,
           machine.template return_value<int>());
}

template <AddressType address_t>
static void syscall_epoll_ctl(Machine<address_t>& machine)
{
	// int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
  [[maybe_unused]] const auto vepoll_fd = machine.template sysarg<int>(0);
  [[maybe_unused]] const auto op = machine.template sysarg<int>(1);
  [[maybe_unused]] const auto vfd = machine.template sysarg<int>(2);
  [[maybe_unused]] const auto g_event = machine.sysarg(3);
  [[maybe_unused]] int real_fd = -1;

  if (machine.has_file_descriptors()) {
    machine.set_result(0);
  } else {
    machine.set_result(-1);
  }
  SYSPRINT("SYSCALL epoll_ctl, epoll_fd: %d  op: %d vfd: %d (real_fd: %d)  event: 0x%lX => %d\n",
		vepoll_fd, op, vfd, real_fd, (long)g_event, (int)machine.return_value());
}

template <AddressType address_t>
static void syscall_epoll_pwait(Machine<address_t>& machine)
{
	//  int epoll_pwait(int epfd, struct epoll_event *events,
	//  				int maxevents, int timeout,
	//  				const sigset_t *sigmask);
  [[maybe_unused]] const auto vepoll_fd = machine.template sysarg<int>(0);
  auto maxevents = machine.template sysarg<int>(2);
	auto timeout = machine.template sysarg<int>(3);
	if (timeout < 0 || timeout > 1) timeout = 1;

	struct epoll_event {
		uint32_t events;
		union {
			void *ptr;
			int fd;
			uint32_t u32;
			uint64_t u64;
		} data;
	};
	std::array<struct epoll_event, 4096> events;
	if (maxevents < 0 || maxevents > (int)events.size()) {
		SYSPRINT("WARNING: Too many epoll events for %d\n", vepoll_fd);
		maxevents = events.size();
	}
  [[maybe_unused]] int real_fd = -1;

  if (machine.has_file_descriptors()) {

		// Finish up: Set -EINTR, then yield
		if (machine.threads().suspend_and_yield(-EINTR)) {
			SYSPRINT("SYSCALL epoll_pwait yielded...\n");
			return;
		}

	} else {
		machine.set_result(-1);
	}
	SYSPRINT("SYSCALL epoll_pwait, epoll_fd: %d (real_fd: %d), maxevents: %d timeout: %d = %ld\n",
		   vepoll_fd, real_fd, maxevents, timeout, (long)machine.return_value());
}
} // namespace riscv
