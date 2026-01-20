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
#if defined(__APPLE__)
#include <time.h>
// struct timespec {
//   time_t tv_sec; /* seconds */
//   long tv_nsec;  /* nanoseconds */
// };
#else
    #include <sys/time.h>
#endif

#include <poll.h>
#include "core/isa/riscv/rv_types.hpp"
#include "sim3/systems/notraced_riscv_isa3_system.hpp"

int poll_with_timeout(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout_ts) {
#if defined(__APPLE__)
  // On macOS, we don't have ppoll, so we need to use poll and then handle the timeout manually.
  int timeout_ms;
  if (timeout_ts != NULL) {
    timeout_ms = timeout_ts->tv_sec * 1000 + timeout_ts->tv_nsec / 1000000;
  } else {
    timeout_ms = -1;
  }
  return poll(fds, nfds, timeout_ms);
#else
  return ppoll(fds, nfds, timeout_ts, NULL);
#endif
}

namespace riscv {
// int ppoll(struct pollfd *fds, nfds_t nfds,
//        const struct timespec *timeout_ts, const sigset_t *sigmask);
template <AddressType address_t>
static void syscall_ppoll(Machine<address_t>& machine)
{
	const auto g_fds = machine.sysarg(0);
	const auto nfds  = machine.template sysarg<unsigned>(1);
	const auto g_ts = machine.sysarg(2);

	struct timespec ts;
	machine.copy_from_guest(&ts, g_ts, sizeof(ts));
	//printf("Timeout from 0x%lX sec=%ld nsec=%ld\n",
	//	(long)g_ts, ts.tv_sec, ts.tv_nsec);

	std::array<struct pollfd, 128> fds;
	std::array<struct pollfd, 128> linux_fds;
	if (nfds > fds.size()) {
		printf("WARNING: Too many ppoll fds: %u\n", nfds);
		machine.set_result_or_error(-1);
		return;
	}

	if (machine.has_file_descriptors()) {
		machine.copy_from_guest(fds.data(), g_fds, nfds * sizeof(fds[0]));
		// Translate to local FD
		for (unsigned i = 0; i < nfds; i++) {
			linux_fds[i].events = fds[i].events;
			linux_fds[i].fd = machine.fds().translate(fds[i].fd);
		}

	    const int res = poll_with_timeout(linux_fds.data(), nfds, &ts);
		// The ppoll system call modifies TS
		//clock_gettime(CLOCK_MONOTONIC, &ts);
		//machine.copy_to_guest(g_ts, &ts, sizeof(ts));

		if (res > 0) {
			// Translate back to virtual FD
			for (unsigned i = 0; i < nfds; i++) {
				fds[i].revents = linux_fds[i].revents;
			}
			machine.copy_to_guest(g_fds, fds.data(), nfds * sizeof(fds[0]));
		}
		machine.set_result_or_error(res);
	} else {
		// No file descriptors, just return 0
		machine.set_result(0);
	}
#if defined(SYSCALL_VERBOSE)
	const auto res = (long)machine.return_value();
	const char *info;
	if (res < 0) info = "error";
	else if (res == 0) info = "timeout";
	else info = "good";
	printf("SYSCALL ppoll, nfds: %u = %ld (%s)\n",
		   nfds, res, info);
#endif
}
} // namespace riscv
