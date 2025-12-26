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
#include "signals.hpp"
#include "../../notraced_riscv_isa3_system.hpp"
#include "sim3/common_macros.hpp"

namespace riscv {
template <AddressType address_t> Signals<address_t>::Signals() {}
template <AddressType address_t> Signals<address_t>::~Signals() {}

template <AddressType address_t> SignalAction<address_t> &Signals<address_t>::get(int sig) {
  if (sig > 0) return _signals.at(sig - 1);
  throw MachineException(ILLEGAL_OPERATION, "Signal 0 invoked");
}

template <AddressType address_t> void Signals<address_t>::enter(Machine<address_t> &machine, int sig) {
  if (sig == 0) return;

  auto &sigact = _signals.at(sig);
  if (sigact.altstack) {
    auto *thread = machine.threads().get_thread();
    // Change to alternate per-thread stack
    auto &stack = per_thread(thread->tid).stack;
    machine.cpu.reg(REG_SP) = stack.ss_sp + stack.ss_size;
  }
  // We have to jump to handler-4 because we are mid-instruction
  // WARNING: Assumption.
  machine.cpu.jump(sigact.handler - 4);
}
template struct Signals<uint32_t>;
template struct Signals<uint64_t>;
} // namespace riscv
