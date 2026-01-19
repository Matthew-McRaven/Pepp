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
#include <chrono>
#include "../notraced_riscv_isa3_system.hpp"
#include "core/isa/riscv/rv_types.hpp"

namespace riscv {
// machine_defaults.cpp
// Default: Stdout allowed
template <AddressType address_t>
void Machine<address_t>::default_printer(const Machine<address_t> &, const char *buffer, size_t len) {
  std::ignore = ::write(1, buffer, len);
}
// Default: Stdin *NOT* allowed
template <AddressType address_t>
long Machine<address_t>::default_stdin(const Machine<address_t> &, char * /*buffer*/, size_t /*len*/) {
  return 0;
}

// Default: RDTIME produces monotonic time with *microsecond*-granularity
template <AddressType address_t> uint64_t Machine<address_t>::default_rdtime(const Machine<address_t> &machine) {
#ifdef __wasm__
  return 0;
#else
  auto now = std::chrono::steady_clock::now();
  auto micros = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
  if (!(machine.has_file_descriptors() && machine.fds().proxy_mode)) micros &= ANTI_FINGERPRINTING_MASK_MICROS();
  return micros;
#endif
}

} // namespace riscv
