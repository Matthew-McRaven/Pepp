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
#include "core/isa/riscv/rv_types.hpp"

#define AMOSIZE_W 0x2
#define AMOSIZE_D 0x3
#define AMOSIZE_Q 0x4

namespace riscv
{
template <AddressType address_t> struct AtomicMemory {
  bool load_reserve(int size, address_t addr) {
    if (!check_alignment(size, addr)) return false;

    m_reservation = addr;
    return true;
  }

  // Volume I: RISC-V Unprivileged ISA V20190608 p.49:
  // An SC can only pair with the most recent LR in program order.
  bool store_conditional(int size, address_t addr) {
    if (!check_alignment(size, addr)) return false;

    bool result = m_reservation == addr;
    // Regardless of success or failure, executing an SC.W
    // instruction invalidates any reservation held by this hart.
    m_reservation = 0x0;
    return result;
  }

private:
  inline bool check_alignment(int size, address_t addr) { return (addr & (size - 1)) == 0; }

  address_t m_reservation = 0x0;
};
}
