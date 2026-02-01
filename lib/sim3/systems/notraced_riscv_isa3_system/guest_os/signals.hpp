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
#include <map>
#include <set>
#include "core/arch/riscv/isa/rv_base.hpp"
#include "core/arch/riscv/isa/rv_types.hpp"
#include "sim3/cores/riscv/registers.hpp"

namespace riscv {
template <AddressType> struct Machine;
template <AddressType> struct Registers;

template <AddressType address_t> struct SignalStack {
  address_t ss_sp = 0x0;
  int ss_flags = 0x0;
  address_t ss_size = 0;
};

template <AddressType address_t> struct SignalAction {
  static constexpr address_t SIG_UNSET = ~(address_t)0x0;
  bool is_unset() const noexcept { return handler == 0x0 || handler == SIG_UNSET; }
  address_t handler = SIG_UNSET;
  bool altstack = false;
  unsigned mask = 0x0;
};

template <AddressType address_t> struct SignalReturn {
  Registers<address_t> regs;
};

template <AddressType address_t> struct SignalPerThread {
  SignalStack<address_t> stack;
  SignalReturn<address_t> sigret;
};

template <AddressType address_t> struct Signals {
  SignalAction<address_t> &get(int sig);
  void enter(Machine<address_t> &, int sig);

  // TODO: Lock this in the future, for multiproessing
  auto& per_thread(int tid) { return m_per_thread[tid]; }

	Signals();
	~Signals();
private:
  std::array<SignalAction<address_t>, 64> _signals{};
  std::map<int, SignalPerThread<address_t>> m_per_thread;
};
} // riscv
