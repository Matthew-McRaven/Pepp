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
#include "../paged_pool.hpp"
#include "sim3/common_macros.hpp"

namespace riscv {
template <AddressType address_t> address_t Memory<address_t>::mmap_allocate(address_t bytes) {
  // Bytes rounded up to nearest PageSize.
  const address_t result = this->m_mmap_address;
  this->m_mmap_address += (bytes + PageMask) & ~address_t{PageMask};
  return result;
}

template <AddressType address_t>
bool Memory<address_t>::mmap_relax(address_t addr, address_t size, address_t new_size) {
  // Undo or relax the last mmap allocation. Returns true if successful.
  if (this->m_mmap_address == addr + size && new_size <= size) {
    this->m_mmap_address = (addr + new_size + PageMask) & ~address_t{PageMask};
    return true;
  }
  return false;
}

template <AddressType address_t> bool Memory<address_t>::mmap_unmap(address_t addr, address_t size) {
  size = (size + PageMask) & ~address_t{PageMask};
  const bool relaxed = this->mmap_relax(addr, size, 0u);
  if (relaxed) {
    // If relaxation happened, invalidate intersecting cache entries.
    this->mmap_cache().invalidate(addr, size);
  } else if (addr >= this->mmap_start()) {
    // If relaxation didn't happen, put in the cache for later.
    this->mmap_cache().insert(addr, size);
  }
  return relaxed;
}
} // namespace riscv
