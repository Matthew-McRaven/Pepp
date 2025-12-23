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
#include <cstdint>
#include "enums/isa/rv_types.hpp"

namespace riscv
{
template <AddressType address_t> struct Machine;

// In fastsim mode the instruction counter becomes a register
// the function, and we only update m_counter in Machine on exit
// When binary translation is enabled we cannot do this optimization.
struct InstrCounter {
  InstrCounter(uint64_t icounter, uint64_t maxcounter) : m_counter(icounter), m_max(maxcounter) {}
  ~InstrCounter() = default;

  template <AddressType address_t> void apply(Machine<address_t> &machine) {
    machine.set_instruction_counter(m_counter);
    machine.set_max_instructions(m_max);
  }
  template <AddressType address_t> void apply_counter(Machine<address_t> &machine) {
    machine.set_instruction_counter(m_counter);
  }
  // Used by binary translator to compensate for its own function already being counted
  // TODO: Account for this inside the binary translator instead. Very minor impact.
  template <AddressType address_t> void apply_counter_minus_1(Machine<address_t> &machine) {
    machine.set_instruction_counter(m_counter - 1);
    machine.set_max_instructions(m_max);
  }
  template <AddressType address_t> void retrieve_max_counter(Machine<address_t> &machine) {
    m_max = machine.max_instructions();
  }
  template <AddressType address_t> void retrieve_counters(Machine<address_t> &machine) {
    m_counter = machine.instruction_counter();
    m_max = machine.max_instructions();
  }

  uint64_t value() const noexcept { return m_counter; }
  uint64_t max() const noexcept { return m_max; }
  void stop() noexcept {
    m_max = 0; // This stops the machine
  }
  void set_counters(uint64_t value, uint64_t max) {
    m_counter = value;
    m_max = max;
  }
  void increment_counter(uint64_t cnt) { m_counter += cnt; }
  bool overflowed() const noexcept { return m_counter >= m_max; }

private:
  uint64_t m_counter;
  uint64_t m_max;
};
} // riscv
