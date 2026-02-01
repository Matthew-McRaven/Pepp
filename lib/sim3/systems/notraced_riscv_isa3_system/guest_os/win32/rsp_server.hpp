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
#include <type_traits>
typedef std::make_signed_t<size_t> ssize_t;

#include <sys/types.h>
#include "core/arch/riscv/isa/rv_types.hpp"
#include "ws2.hpp"

namespace riscv {

template <AddressType address_t>
RSP<address_t>::RSP(riscv::Machine<address_t>& m, uint16_t port)
	: m_machine{m}
{
    ws2::init();

	this->server_fd = socket(AF_INET, SOCK_STREAM, 0);
    u_long mode = 1;
    ioctlsocket(server_fd, FIONBIO, &mode); // SOCK_NONBLOCK

	int opt = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_BROADCAST, // SO_BROADCAST = SO_REUSEPORT
        (const char*)&opt, sizeof(opt))) {
        closesocket(server_fd);
		throw MachineException(SYSTEM_CALL_FAILED, "Failed to enable REUSEADDR/PORT");
	}
	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);
	if (bind(server_fd, (struct sockaddr*) &address,
			sizeof(address)) < 0) {
        closesocket(server_fd);
		throw MachineException(SYSTEM_CALL_FAILED, "GDB listener failed to bind to port");
	}
	if (listen(server_fd, 2) < 0) {
        closesocket(server_fd);
		throw MachineException(SYSTEM_CALL_FAILED, "GDB listener failed to listen on port");
	}
}
template <AddressType address_t>
std::unique_ptr<RSPClient<address_t>> RSP<address_t>::accept(int timeout_secs)
{
	struct timeval tv {
		.tv_sec = timeout_secs,
		.tv_usec = 0
	};
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(server_fd, &fds);

	const int ret = select(server_fd + 1, &fds, NULL, NULL, &tv);
	if (ret <= 0) {
		return nullptr;
	}

	struct sockaddr_in address;
	int addrlen = sizeof(address);
	int sockfd = ::accept(server_fd, (struct sockaddr*) &address,
			(socklen_t*) &addrlen);
	if (sockfd < 0) {
		return nullptr;
	}
	// Disable Nagle
	int opt = 1;
	if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof(opt))) {
        closesocket(sockfd);
		return nullptr;
	}
	// Enable receive and send timeouts
	DWORD timeout = 60*1000;
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO | SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout))) {
        closesocket(sockfd);
		return nullptr;
	}
	return std::make_unique<RSPClient<address_t>>(m_machine, sockfd);
}
template <AddressType address_t> inline
RSP<address_t>::~RSP() {
    closesocket(server_fd);
}

template <AddressType address_t> inline
RSPClient<address_t>::~RSPClient() {
    if (!is_closed())
        closesocket(this->sockfd);
}

template <AddressType address_t> inline
void RSPClient<address_t>::close_now() {
    this->m_closed = true;
    closesocket(this->sockfd);
}

template <AddressType address_t>
bool RSPClient<address_t>::sendf(const char* fmt, ...)
{
    char buffer[PACKET_SIZE];
    va_list args;
    va_start(args, fmt);
    int plen = forge_packet(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    if (UNLIKELY(m_verbose)) {
        printf("TX >>> %.*s\n", plen, buffer);
    }
    int len =  ::send(sockfd, buffer, plen, 0);
    if (len <= 0) {
        this->close_now();
        return false;
    }
    // Acknowledgement
    int rlen = ::recv(sockfd, buffer, 1, 0);
    if (rlen <= 0) {
        this->close_now();
        return false;
    }
    return (buffer[0] == '+');
}

template <AddressType address_t>
bool RSPClient<address_t>::send(const char* str)
{
    char buffer[PACKET_SIZE];
    int plen = forge_packet(buffer, sizeof(buffer), str, strlen(str));
    if (UNLIKELY(m_verbose)) {
        printf("TX >>> %.*s\n", plen, buffer);
    }
    int len = ::write(sockfd, buffer, plen);
    if (len <= 0) {
        this->close_now();
        return false;
    }
    // Acknowledgement
    int rlen = ::recv(sockfd, buffer, 1, 0);
    if (rlen <= 0) {
        this->close_now();
        return false;
    }
    return (buffer[0] == '+');
}
template <AddressType address_t>
bool RSPClient<address_t>::process_one()
{
    char tmp[1024];
    int len = ::recv(this->sockfd, tmp, sizeof(tmp), 0);
    if (len <= 0) {
        this->close_now();
        return false;
    }
    if (UNLIKELY(m_verbose)) {
        printf("RX <<< %.*s\n", len, tmp);
    }
    for (int i = 0; i < len; i++)
    {
        char c = tmp[i];
        if (buffer.empty() && c == '+') {
            /* Ignore acks? */
        }
        else if (c == '$') {
            this->buffer.clear();
        }
        else if (c == '#') {
            reply_ack();
            process_data();
            this->buffer.clear();
            i += 2;
        }
        else {
            this->buffer.append(&c, 1);
            if (buffer.size() >= PACKET_SIZE)
                break;
        }
    }
    return true;
}

template <AddressType address_t> inline
void RSPClient<address_t>::reply_ack() {
    ssize_t len = ::send(sockfd, "+", 1, 0);
    if (len < 0) throw MachineException(SYSTEM_CALL_FAILED, "RSPClient: Unable to ACK");
}

template <AddressType address_t>
void RSPClient<address_t>::kill() {
    closesocket(sockfd);
}

} // riscv
