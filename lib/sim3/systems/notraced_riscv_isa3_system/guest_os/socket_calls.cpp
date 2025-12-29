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
#include "./sysprint.hpp"
#include "sim3/subsystems/ram/paged_pool.hpp"
#include "sim3/systems/notraced_riscv_isa3_system.hpp"

#ifdef WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "win32/ws2.hpp"
WSADATA riscv::ws2::global_winsock_data;
bool riscv::ws2::winsock_initialized = false;
using ssize_t = long long int;
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#if __has_include(<linux/netlink.h>)
#define HAVE_LINUX_NETLINK
#include <linux/netlink.h>
#endif
#endif

namespace riscv {

template <AddressType address_t>
struct guest_iovec {
	address_t iov_base;
	address_t iov_len;
};
template <AddressType address_t>
struct guest_msghdr
{
	address_t msg_name;		/* Address to send to/receive from.  */
	uint32_t        msg_namelen;	/* Length of address data.  */

	address_t msg_iov;		/* Vector of data to send/receive into.  */
	address_t msg_iovlen;		/* Number of elements in the vector.  */

	address_t msg_control;	/* Ancillary data (eg BSD filedesc passing). */
	address_t msg_controllen;	/* Ancillary data buffer length. */
	int             msg_flags;		/* Flags on received message.  */
};

#ifdef SOCKETCALL_VERBOSE
template <AddressType address_t>
static void print_address(std::array<char, 128> buffer, address_t addrlen)
{
	if (addrlen < 12)
		throw MachineException(INVALID_PROGRAM, "Socket address too small", addrlen);

	char printbuf[INET6_ADDRSTRLEN];
	auto* sin6 = (struct sockaddr_in6 *)buffer.data();

	switch (sin6->sin6_family) {
	case AF_INET6:
		inet_ntop(AF_INET6, &sin6->sin6_addr, printbuf, sizeof(printbuf));
		printf("SYSCALL -- IPv6 address: %s\n", printbuf);
		break;
	case AF_INET: {
		auto* sin4 = (struct sockaddr_in *)buffer.data();
		inet_ntop(AF_INET, &sin4->sin_addr, printbuf, sizeof(printbuf));
		printf("SYSCALL -- IPv4 address: %s\n", printbuf);
		break;
	}
	case AF_UNIX: {
		auto* sun = (struct sockaddr_un *)buffer.data();
		printf("SYSCALL -- UNIX address: %s\n", sun->sun_path);
		break;
	}
#ifdef HAVE_LINUX_NETLINK
	case AF_NETLINK: {
		auto* snl = (struct sockaddr_nl *)buffer.data();
		printf("SYSCALL -- NetLink port: %u\n", snl->nl_pid);
		break;
	}
#endif
	}
}
#endif

template <AddressType address_t>
static void syscall_socket(Machine<address_t>& machine)
{
	const auto [domain, type, proto] =
		machine.template sysargs<int, int, int> ();
	int real_fd = -1;

	if (machine.has_file_descriptors() && machine.fds().permit_sockets) {
#ifdef WIN32
        ws2::init();
#endif
		real_fd = socket(domain, type, proto);
		if (real_fd > 0) {
			const int vfd = machine.fds().assign_socket(real_fd);
			machine.set_result(vfd);
		} else {
			// Translate errno() into kernel API return value
			machine.set_result(-errno);
		}
	} else {
		machine.set_result(-EBADF);
	}
#ifdef SOCKETCALL_VERBOSE
	const char* domname;
	switch (domain & 0xFF) {
		case AF_UNIX: domname = "Unix"; break;
		case AF_INET: domname = "IPv4"; break;
		case AF_INET6: domname = "IPv6"; break;
#ifdef HAVE_LINUX_NETLINK
		case AF_NETLINK: domname = "Netlink"; break;
#endif
		default: domname = "unknown";
	}
	const char* typname;
	switch (type & 0xFF) {
		case SOCK_STREAM: typname = "Stream"; break;
		case SOCK_DGRAM: typname = "Datagram"; break;
		case SOCK_SEQPACKET: typname = "Seq.packet"; break;
		case SOCK_RAW: typname = "Raw"; break;
		default: typname = "unknown";
	}
	SYSPRINT("SYSCALL socket, domain: %x (%s) type: %x (%s) proto: %x = %d (real fd: %d)\n",
		domain, domname, type, typname, proto, (int)machine.return_value(), real_fd);
#endif
}

template <AddressType address_t>
static void syscall_bind(Machine<address_t>& machine)
{
	const auto [vfd, g_addr, addrlen] =
		machine.template sysargs<int, address_t, address_t> ();

	alignas(16) std::array<char, 128> buffer;
	if (addrlen > buffer.size()) {
		machine.set_result(-ENOMEM);
		return;
	}

	if (machine.has_file_descriptors() && machine.fds().permit_sockets) {

		const auto real_fd = machine.fds().translate(vfd);
		machine.copy_from_guest(buffer.data(), g_addr, addrlen);

	#ifdef SOCKETCALL_VERBOSE
		print_address<address_t>(buffer, addrlen);
	#endif

		int res = bind(real_fd, (struct sockaddr *)buffer.data(), addrlen);
		machine.set_result_or_error(res);
	} else {
		machine.set_result(-EBADF);
	}

	SYSPRINT("SYSCALL bind, vfd: %d addr: 0x%lX len: 0x%lX = %d\n",
		vfd, (long)g_addr, (long)addrlen, (int)machine.return_value());
}

template <AddressType address_t>
static void syscall_listen(Machine<address_t>& machine)
{
	const auto [vfd, backlog] =
		machine.template sysargs<int, int> ();

	SYSPRINT("SYSCALL listen, vfd: %d backlog: %d\n",
		vfd, backlog);

	if (machine.has_file_descriptors() && machine.fds().permit_sockets) {

		const auto real_fd = machine.fds().translate(vfd);

		int res = listen(real_fd, backlog);
		machine.set_result_or_error(res);
		return;
	}
	machine.set_result(-EBADF);
}

template <AddressType address_t>
static void syscall_accept(Machine<address_t>& machine)
{
	const auto [vfd, g_addr, g_addrlen] =
		machine.template sysargs<int, address_t, address_t> ();

	SYSPRINT("SYSCALL accept, vfd: %d addr: 0x%lX\n",
		vfd, (long)g_addr);

	if (machine.has_file_descriptors() && machine.fds().permit_sockets) {

		const auto real_fd = machine.fds().translate(vfd);
		alignas(16) char buffer[128];
		socklen_t addrlen = sizeof(buffer);

		int res = accept(real_fd, (struct sockaddr *)buffer, &addrlen);
		if (res >= 0) {
			// Assign and translate the new fd to virtual fd
			res = machine.fds().assign_socket(res);
			machine.copy_to_guest(g_addr, buffer, addrlen);
			machine.copy_to_guest(g_addrlen, &addrlen, sizeof(addrlen));
		}
		machine.set_result_or_error(res);
		return;
	}
	machine.set_result(-EBADF);
}

template <AddressType address_t>
static void syscall_connect(Machine<address_t>& machine)
{
	const auto [vfd, g_addr, addrlen] =
		machine.template sysargs<int, address_t, address_t> ();
	alignas(16) std::array<char, 128> buffer;

	if (addrlen > buffer.size()) {
		machine.set_result(-ENOMEM);
		return;
	}
	int real_fd = -EBADF;

	if (machine.has_file_descriptors() && machine.fds().permit_sockets) {

		real_fd = machine.fds().translate(vfd);
		machine.copy_from_guest(buffer.data(), g_addr, addrlen);

#ifdef SOCKETCALL_VERBOSE
		print_address<address_t>(buffer, addrlen);
#endif

		const int res = connect(real_fd, (const struct sockaddr *)buffer.data(), addrlen);
		machine.set_result_or_error(res);
	} else {
		machine.set_result(-EBADF);
	}

	SYSPRINT("SYSCALL connect, vfd: %d (real_fd: %d) addr: 0x%lX len: %zu = %ld\n",
		vfd, real_fd, (long)g_addr, (size_t)addrlen, (long)machine.return_value());
}

template <AddressType address_t>
static void syscall_getsockname(Machine<address_t>& machine)
{
	const auto [vfd, g_addr, g_addrlen] =
		machine.template sysargs<int, address_t, address_t> ();

	if (machine.has_file_descriptors() && machine.fds().permit_sockets)
	{
		const auto real_fd = machine.fds().translate(vfd);

		struct sockaddr addr {};
		socklen_t addrlen = 0;
		int res = getsockname(real_fd, &addr, &addrlen);
		if (res == 0) {
			machine.copy_to_guest(g_addr, &addr, addrlen);
			machine.copy_to_guest(g_addrlen, &addrlen, sizeof(addrlen));
		}
		machine.set_result_or_error(res);
	} else {
		machine.set_result(-EBADF);
	}

	SYSPRINT("SYSCALL getsockname, fd: %d addr: 0x%lX len: 0x%lX = %ld\n",
		vfd, (long)g_addr, (long)g_addrlen, (long)machine.return_value());
}

template <AddressType address_t>
static void syscall_getpeername(Machine<address_t>& machine)
{
	const auto [vfd, g_addr, g_addrlen] =
		machine.template sysargs<int, address_t, address_t> ();

	if (machine.has_file_descriptors() && machine.fds().permit_sockets)
	{
		const auto real_fd = machine.fds().translate(vfd);

		struct sockaddr addr {};
		socklen_t addrlen = 0;
		int res = getpeername(real_fd, &addr, &addrlen);

		if (res == 0) {
			machine.copy_to_guest(g_addr, &addr, addrlen);
			machine.copy_to_guest(g_addrlen, &addrlen, sizeof(addrlen));
		}
		machine.set_result_or_error(res);
	} else {
		machine.set_result(-EBADF);
	}

	SYSPRINT("SYSCALL getpeername, fd: %d addr: 0x%lX len: 0x%lX = %ld\n",
		vfd, (long)g_addr, (long)g_addrlen, (long)machine.return_value());
}

template <AddressType address_t>
static void syscall_sendto(Machine<address_t>& machine)
{
	// ssize_t sendto(int vfd, const void *buf, size_t len, int flags,
	//		   const struct sockaddr *dest_addr, socklen_t addrlen);
	const auto [vfd, g_buf, buflen, flags, g_dest_addr, dest_addrlen] =
		machine.template sysargs<int, address_t, address_t, int, address_t, unsigned>();
	int real_fd = -1;

	if (dest_addrlen > 128) {
		machine.set_result(-ENOMEM);
		return;
	}
	alignas(16) char dest_addr[128];
	machine.copy_from_guest(dest_addr, g_dest_addr, dest_addrlen);

	if (machine.has_file_descriptors() && machine.fds().permit_sockets) {

		real_fd = machine.fds().translate(vfd);

#ifdef __linux__
		// Gather up to 1MB of pages we can read into
		std::array<riscv::vBuffer, 256> buffers;
		const size_t buffer_cnt =
			machine.memory.gather_buffers_from_range(buffers.size(), buffers.data(), g_buf, buflen);

		struct msghdr msg;
		msg.msg_name = dest_addr;
		msg.msg_namelen = static_cast<socklen_t>(dest_addrlen);
		msg.msg_iov = (struct iovec *)buffers.data();
		msg.msg_iovlen = buffer_cnt;
		msg.msg_control = nullptr;
		msg.msg_controllen = 0;
		msg.msg_flags = 0;

		const ssize_t res = sendmsg(real_fd, &msg, flags);
#else
		// XXX: Write me
		(void)real_fd;
		const ssize_t res = -1;
#endif
		machine.set_result_or_error(res);
	} else {
		machine.set_result(-EBADF);
	}
	SYSPRINT("SYSCALL sendto, fd: %d (real fd: %d) len: %ld flags: %#x = %ld\n",
			 vfd, real_fd, (long)buflen, flags, (long)machine.return_value());
}

template <AddressType address_t>
static void syscall_recvfrom(Machine<address_t>& machine)
{
	// ssize_t recvfrom(int vfd, void *buf, size_t len, int flags,
	// 					struct sockaddr *src_addr, socklen_t *addrlen);
	const auto [vfd, g_buf, buflen, flags, g_src_addr, g_addrlen] =
		machine.template sysargs<int, address_t, address_t, int, address_t, address_t>();
	int real_fd = -1;

	if (machine.has_file_descriptors() && machine.fds().permit_sockets) {

		real_fd = machine.fds().translate(vfd);

#ifdef __linux__
		// Gather up to 1MB of pages we can read into
		std::array<riscv::vBuffer, 256> buffers;
		const size_t buffer_cnt =
			machine.memory.gather_writable_buffers_from_range(buffers.size(), buffers.data(), g_buf, buflen);

		alignas(16) char dest_addr[128];
		struct msghdr hdr;
		hdr.msg_name = dest_addr;
		hdr.msg_namelen = sizeof(dest_addr);
		hdr.msg_iov = (struct iovec *)buffers.data();
		hdr.msg_iovlen = buffer_cnt;
		hdr.msg_control = nullptr;
		hdr.msg_controllen = 0;
		hdr.msg_flags = 0;
	#if 0
		printf("recvfrom(buffers: %zu, total: %zu)\n", buffer_cnt, size_t(buflen));
	#endif

		const ssize_t res = recvmsg(real_fd, &hdr, flags);
		if (res >= 0) {
			if (g_src_addr != 0x0)
				machine.copy_to_guest(g_src_addr, hdr.msg_name, hdr.msg_namelen);
			if (g_addrlen != 0x0)
				machine.copy_to_guest(g_addrlen, &hdr.msg_namelen, sizeof(hdr.msg_namelen));
		}
#else
		// XXX: Write me
		(void)real_fd;
		const ssize_t res = -1;
#endif
		machine.set_result_or_error(res);
	} else {
		machine.set_result(-EBADF);
	}
	SYSPRINT("SYSCALL recvfrom, fd: %d (real fd: %d) len: %ld flags: %#x = %ld\n",
			 vfd, real_fd, (long)buflen, flags, (long)machine.return_value());
}

template <AddressType address_t>
static void syscall_recvmsg(Machine<address_t>& machine)
{
	// ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags)
	const auto [vfd, g_msg, flags] =
		machine.template sysargs<int, address_t, int>();
	int real_fd = -1;

	if (machine.has_file_descriptors() && machine.fds().permit_sockets) {

		real_fd = machine.fds().translate(vfd);

#ifdef __linux__
		std::array<riscv::vBuffer, 256> buffers;
		std::array<guest_iovec<address_t>, 256> g_iov;
		guest_msghdr<address_t> msg;
		machine.copy_from_guest(&msg, g_msg, sizeof(msg));

		if (msg.msg_iovlen > g_iov.size()) {
			machine.set_result(-ENOMEM);
			return;
		}
		machine.copy_from_guest(g_iov.data(), msg.msg_iov, msg.msg_iovlen * sizeof(guest_iovec<address_t>));

		unsigned vec_cnt = 0;
		size_t total = 0;
		for (unsigned i = 0; i < msg.msg_iovlen; i++) {
			const address_t g_buf = g_iov[i].iov_base;
			const address_t g_len = g_iov[i].iov_len;
			vec_cnt +=
				machine.memory.gather_writable_buffers_from_range(buffers.size() - vec_cnt, &buffers[vec_cnt], g_buf, g_len);
			total += g_len;
		}
	#if 0
		printf("recvmsg(buffers: %u, total: %zu)\n", vec_cnt, total);
	#endif

		alignas(16) char dest_addr[128];
		struct msghdr hdr;
		hdr.msg_name = dest_addr;
		hdr.msg_namelen = sizeof(dest_addr);
		hdr.msg_iov = (struct iovec *)buffers.data();
		hdr.msg_iovlen = vec_cnt;
		hdr.msg_control = nullptr;
		hdr.msg_controllen = 0;
		hdr.msg_flags = msg.msg_flags;

		const ssize_t res = recvmsg(real_fd, &hdr, flags);
		if (res >= 0) {
			if (msg.msg_name != 0x0) {
				machine.copy_to_guest(msg.msg_name, hdr.msg_name, hdr.msg_namelen);
				msg.msg_namelen = hdr.msg_namelen;
			}
		}
#else
		// XXX: Write me
		(void)real_fd;
		const ssize_t res = -1;
#endif
		machine.set_result_or_error(res);
	} else {
		machine.set_result(-EBADF);
	}
	SYSPRINT("SYSCALL recvmsg, fd: %d (real fd: %d) msg: 0x%lX flags: %#x = %ld\n",
			 vfd, real_fd, (long)g_msg, flags, (long)machine.return_value());
}

template <AddressType address_t>
static void syscall_sendmmsg(Machine<address_t>& machine)
{
	// ssize_t sendmmsg(int vfd, struct mmsghdr *msgvec, unsigned int vlen, int flags);
	const auto [vfd, g_msgvec, veclen, flags] =
		machine.template sysargs<int, address_t, unsigned, int>();
	int real_fd = -1;

	if (machine.has_file_descriptors() && machine.fds().permit_sockets) {

		real_fd = machine.fds().translate(vfd);

#ifdef __linux__
		std::array<struct mmsghdr, 128> msgvec;

		if (veclen > msgvec.size()) {
			machine.set_result(-ENOMEM);
			return;
		}
		machine.copy_from_guest(&msgvec[0], g_msgvec, veclen * sizeof(msgvec[0]));

		ssize_t finalcnt = 0;
		for (size_t i = 0; i < veclen; i++)
		{
			auto& entry = msgvec[i].msg_hdr;
			std::array<guest_iovec<address_t>, 128> g_iov;

			if (entry.msg_iovlen > g_iov.size()) {
				machine.set_result(-ENOMEM);
				return;
			}
			machine.copy_from_guest(&g_iov[0], (address_t)uintptr_t(entry.msg_iov), entry.msg_iovlen * sizeof(guest_iovec<address_t>));

			std::array<riscv::vBuffer, 128> buffers;
			unsigned vec_cnt = 0;
			size_t total_bytes = 0;
			for (unsigned i = 0; i < entry.msg_iovlen; i++) {
				const address_t g_buf = g_iov[i].iov_base;
				const address_t g_len = g_iov[i].iov_len;
				vec_cnt +=
					machine.memory.gather_buffers_from_range(buffers.size() - vec_cnt, &buffers[vec_cnt], g_buf, g_len);
				total_bytes += g_len;
			}

		#ifdef SOCKETCALL_VERBOSE
			printf("SYSCALL -- Vec %zu: Buffers: %u  msg_name=%p namelen: %u\n",
				i, vec_cnt, entry.msg_name, entry.msg_namelen);
		#endif

			struct msghdr hdr;
			hdr.msg_name = nullptr;
			hdr.msg_namelen = 0;
			hdr.msg_iov = (struct iovec *)buffers.data();
			hdr.msg_iovlen = vec_cnt;
			hdr.msg_control = nullptr;
			hdr.msg_controllen = 0;
			hdr.msg_flags = entry.msg_flags;

			const ssize_t res = sendmsg(real_fd, &hdr, flags);
			if (res < 0) {
				finalcnt = res;
				msgvec[i].msg_len = 0;
				break;
			} else if (res > 0) {
				finalcnt += 1;
				msgvec[i].msg_len = res;
			}
		}
		if (finalcnt > 0) {
			machine.copy_to_guest(g_msgvec, &msgvec[0], finalcnt * sizeof(msgvec[0]));
		}
#else
		// XXX: Write me
		(void)real_fd;
		const ssize_t finalcnt = 0;
#endif
		machine.set_result_or_error(finalcnt);
	} else {
		machine.set_result(-EBADF);
	}
	SYSPRINT("SYSCALL sendmmsg, fd: %d (real fd: %d) msgvec: 0x%lX flags: %#x = %ld\n",
			 vfd, real_fd, (long)g_msgvec, flags, (long)machine.return_value());
}

template <AddressType address_t>
static void syscall_setsockopt(Machine<address_t>& machine)
{
	const auto [vfd, level, optname, g_opt, optlen] =
		machine.template sysargs<int, int, int, address_t, unsigned> ();

	if (optlen > 128) {
		machine.set_result(-ENOMEM);
		return;
	}

	if (machine.has_file_descriptors() && machine.fds().permit_sockets) {

		const auto real_fd = machine.fds().translate(vfd);
		alignas(8) char buffer[128];
		machine.copy_from_guest(buffer, g_opt, optlen);

		int res = setsockopt(real_fd, level, optname, buffer, optlen);
		machine.set_result_or_error(res);
	} else {
		machine.set_result(-EBADF);
	}
	SYSPRINT("SYSCALL setsockopt, fd: %d level: %x optname: %#x len: %u = %ld\n",
		vfd, level, optname, optlen, (long)machine.return_value());
}

template <AddressType address_t>
static void syscall_getsockopt(Machine<address_t>& machine)
{
	const auto [vfd, level, optname, g_opt, g_optlen] =
		machine.template sysargs<int, int, int, address_t, address_t> ();
	socklen_t optlen = 0;

	if (machine.has_file_descriptors() && machine.fds().permit_sockets)
	{
		const auto real_fd = machine.fds().translate(vfd);

		alignas(8) char buffer[128];
		optlen = std::min(sizeof(buffer), size_t(g_optlen));
		int res = getsockopt(real_fd, level, optname, buffer, &optlen);
		if (res == 0) {
			machine.copy_to_guest(g_optlen, &optlen, sizeof(optlen));
			machine.copy_to_guest(g_opt, buffer, optlen);
		}
		machine.set_result_or_error(res);
	} else {
		machine.set_result(-EBADF);
	}

	SYSPRINT("SYSCALL getsockopt, fd: %d level: %x optname: %#x len: %ld/%ld = %ld\n",
			 vfd, level, optname, (long)optlen, (long)g_optlen, (long)machine.return_value());
}

template <AddressType address_t>
void add_socket_syscalls(Machine<address_t>& machine)
{
	machine.install_syscall_handler(198, syscall_socket<address_t>);
	machine.install_syscall_handler(200, syscall_bind<address_t>);
	machine.install_syscall_handler(201, syscall_listen<address_t>);
	machine.install_syscall_handler(202, syscall_accept<address_t>);
	machine.install_syscall_handler(203, syscall_connect<address_t>);
	machine.install_syscall_handler(204, syscall_getsockname<address_t>);
	machine.install_syscall_handler(205, syscall_getpeername<address_t>);
	machine.install_syscall_handler(206, syscall_sendto<address_t>);
	machine.install_syscall_handler(207, syscall_recvfrom<address_t>);
	machine.install_syscall_handler(208, syscall_setsockopt<address_t>);
	machine.install_syscall_handler(209, syscall_getsockopt<address_t>);
	machine.install_syscall_handler(212, syscall_recvmsg<address_t>);
	machine.install_syscall_handler(269, syscall_sendmmsg<address_t>);
}

template void add_socket_syscalls<uint32_t>(Machine<uint32_t> &);
template void add_socket_syscalls<uint64_t>(Machine<uint64_t> &);

} // riscv
