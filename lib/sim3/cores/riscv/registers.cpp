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
#include "./registers.hpp"
#include <stdexcept>
#include "enums/isa/rv_base.hpp"

namespace riscv {

template <AddressType address_t> PEPP_COLD_PATH() std::string Registers<address_t>::to_string() const {
  char buffer[600];
  unsigned len = 0;
  for (int i = 1; i < 32; i++) {
    if (UNLIKELY(len >= sizeof(buffer))) throw std::logic_error("Not possible, buffer sized at compile time");
    len += snprintf(buffer + len, sizeof(buffer) - len, "[%s\t%08X] ", RISCV::regname(i), this->get(i));
    if (i % 5 == 4) {
      if (UNLIKELY(len >= sizeof(buffer))) throw std::logic_error("Not possible, buffer sized at compile time");
      len += snprintf(buffer + len, sizeof(buffer) - len, "\n");
    }
  }
  return std::string(buffer, len);
}

template <AddressType address_t> PEPP_COLD_PATH() std::string Registers<address_t>::flp_to_string() const {
  char buffer[800];
  unsigned len = 0;
  for (int i = 0; i < 32; i++) {
    auto &src = this->getfl(i);
    const char T = (src.i32[1] == 0) ? 'S' : 'D';
    if constexpr (true) {
      double val = (src.i32[1] == 0) ? src.f32[0] : src.f64;
      if (UNLIKELY(len >= sizeof(buffer))) throw std::logic_error("Not possible, buffer sized at compile time");
      len += snprintf(buffer + len, sizeof(buffer) - len, "[%s\t%c%+.2f] ", RISCV::flpname(i), T, val);
    } else {
      if (src.i32[1] == 0) {
        double val = src.f64;
        if (UNLIKELY(len >= sizeof(buffer))) throw std::logic_error("Not possible, buffer sized at compile time");
        len += snprintf(buffer + len, sizeof(buffer) - len, "[%s\t%c0x%lX] ", RISCV::flpname(i), T, *(int64_t *)&val);
      } else {
        float val = src.f32[0];
        if (UNLIKELY(len >= sizeof(buffer))) throw std::logic_error("Not possible, buffer sized at compile time");
        len += snprintf(buffer + len, sizeof(buffer) - len, "[%s\t%c0x%X] ", RISCV::flpname(i), T, *(int32_t *)&val);
      }
    }
    if (i % 5 == 4) {
      if (UNLIKELY(len >= sizeof(buffer))) throw std::logic_error("Not possible, buffer sized at compile time");
      len += snprintf(buffer + len, sizeof(buffer) - len, "\n");
    }
  }
  if (UNLIKELY(len >= sizeof(buffer))) throw std::logic_error("Not possible, buffer sized at compile time");
  len += snprintf(buffer + len, sizeof(buffer) - len, "[FFLAGS\t0x%X] ", m_fcsr.fflags);
  return std::string(buffer, len);
}

} // namespace riscv

template struct riscv::Registers<uint32_t>;
template struct riscv::Registers<uint64_t>;
