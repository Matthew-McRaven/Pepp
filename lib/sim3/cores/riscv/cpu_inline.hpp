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
#include "./notraced_cpu.hpp"
#include "bts/isa/rv_types.hpp"

namespace riscv {
template <AddressType address_t>
inline CPU<address_t>::CPU(Machine<address_t> &machine) : m_machine{machine}, m_exec(empty_execute_segment().get()) {}
template <AddressType address_t> inline void CPU<address_t>::reset_stack_pointer() noexcept {
  // initial stack location
  this->reg(2) = machine().memory.stack_initial();
}

template <AddressType address_t> inline void CPU<address_t>::jump(const address_t dst) {
  // it's possible to jump to a misaligned address
  if constexpr (!compressed_enabled) {
    if (UNLIKELY(dst & 0x3)) {
      trigger_exception(MISALIGNED_INSTRUCTION, dst);
    }
  } else {
    if (UNLIKELY(dst & 0x1)) {
      trigger_exception(MISALIGNED_INSTRUCTION, dst);
    }
  }
  this->registers().pc = dst;
}

template <AddressType address_t> inline void CPU<address_t>::aligned_jump(const address_t dst) noexcept {
  this->registers().pc = dst;
}

template <AddressType address_t> inline void CPU<address_t>::increment_pc(int delta) noexcept {
  registers().pc += delta;
}

// Use a trick to access the Machine directly on g++/clang, Linux-only for now
#if (defined(__GNUG__) || defined(__clang__)) && defined(__linux__)
template <AddressType address_t> PEPP_ALWAYS_INLINE inline
Machine<address_t>& CPU<address_t>::machine() noexcept { return *reinterpret_cast<Machine<address_t>*> (this); }
template <AddressType address_t> PEPP_ALWAYS_INLINE inline
const Machine<address_t>& CPU<address_t>::machine() const noexcept { return *reinterpret_cast<const Machine<address_t>*> (this); }
#else
template <AddressType address_t> PEPP_ALWAYS_INLINE inline
Machine<address_t>& CPU<address_t>::machine() noexcept { return this->m_machine; }
template <AddressType address_t> PEPP_ALWAYS_INLINE inline
const Machine<address_t>& CPU<address_t>::machine() const noexcept { return this->m_machine; }
#endif

template <AddressType address_t> PEPP_ALWAYS_INLINE inline
Memory<address_t>& CPU<address_t>::memory() noexcept { return machine().memory; }
template <AddressType address_t> PEPP_ALWAYS_INLINE inline
const Memory<address_t>& CPU<address_t>::memory() const noexcept { return machine().memory; }
} // namespace riscv
