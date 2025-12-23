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
#include <vector>
#include "./instructions/instr_decoder.hpp"
#include "./registers.hpp"
#include "sim3/common_macros.hpp"
#include "sim3/subsystems/ram/paged_pool/page.hpp"

namespace riscv {
template <AddressType address_t> struct Machine;
template <AddressType address_t> struct DecodedExecuteSegment;

template <AddressType address_type> struct alignas(32) CPU {
  using address_t = address_type;
  using format_t = instruction_format; // machine instruction format
  using breakpoint_t = std::function<void(CPU<address_t> &)>;
  using execute_fault_t = void (*)(CPU<address_t> &, const Page &);
  using override_execute_segment_t = DecodedExecuteSegment<address_t> &(*)(CPU<address_t> &);
  using instruction_t = Instruction<address_t>;

  // Dispatch modes (determined at configure-time):
  // 1. Switch-case: Uses a big switch-case for bytecodes. Works and runs everywhere.
  // 2. Threaded: Uses computed gotos to jump around at a faster speed, but
  //    is only supported on GCC and Clang. Very fast dispatch.
  // 3. TCO: Uses musttail to jump around at the fastest speed, but
  //    is only supported on Clang. Very fast dispatch.
  /// Executes RISC-V code using the configure-time dispatch mode.
  /// @param pc The starting address
  /// @param icounter The instruction counter start value (usually 0)
  /// @param maxcounter The instruction limit value (usually several millions)
  /// @return Returns true if the machine stopped normally, otherwise an execution timeout happened.
  bool simulate(address_t pc, uint64_t icounter, uint64_t maxcounter);

  // Step precisely one instruction forward from current PC.
  void step_one(bool use_instruction_counter = true);

  /// @brief Executes one instruction at a time, and can stop at
  // any instruction. Can be used for debugging.
  void simulate_precise();

  /// @brief  Get the current PC
  /// @return The current PC address
  address_t pc() const noexcept { return registers().pc; }
  void increment_pc(int delta) noexcept;
  void jump(address_t);
  void aligned_jump(address_t) noexcept;

  /// @brief Retrieve current register state
  /// @return Current register state
  PEPP_ALWAYS_INLINE auto &registers() noexcept { return this->m_regs; }
  /// @brief Retrieve current register state
  /// @return Current register state
  PEPP_ALWAYS_INLINE const auto &registers() const noexcept { return this->m_regs; }

  int cpu_id() const noexcept { return 0; }

  auto &reg(uint32_t idx) noexcept { return registers().get(idx); }
  const auto &reg(uint32_t idx) const noexcept { return registers().get(idx); }
  auto &cireg(uint16_t idx) noexcept { return registers().get(idx + 0x8); }
  const auto &cireg(uint16_t idx) const noexcept { return registers().get(idx + 0x8); }
  auto &ciflp(uint16_t idx) noexcept { return registers().getfl(idx + 0x8); }

  Machine<address_t> &machine() noexcept;
  const Machine<address_t> &machine() const noexcept;

  Memory<address_t> &memory() noexcept;
  const Memory<address_t> &memory() const noexcept;

		auto& atomics() noexcept { return memory().atomics(); }
		const auto& atomics() const noexcept { return memory().atomics(); }
		template <typename Type>
		void amo(format_t, Type(*op)(CPU&, Type&, uint32_t));

		[[noreturn]]
		static void trigger_exception(int which, address_t = 0) PEPP_COLD_PATH();

		// Directly execute an instruction (given bits)
		void execute(format_t);
		// Directly execute a function by its handler, or if the handler is 0, resolve the handler then execute
		void execute(uint8_t& handler_idx, uint32_t instr);
		// Read the next instruction bits
		format_t read_next_instruction() const;
		// Internal preempt() implementation that executes and restores old registers
    address_t preempt_internal(Registers<address_t> &old_regs, bool Throw, bool store_regs, address_t pc,
                               uint64_t max_instr);

    // Pretty print instructions
    std::string to_string(format_t format) const;
    std::string to_string(format_t format, const instruction_t &instr) const;

		/// @brief Pretty-print the current instruction
		/// @return Returns a formatted string of the current instruction
		std::string current_instruction_to_string() const;

		format_t read_next_instruction_slowpath() const PEPP_COLD_PATH();
    static const instruction_t &decode(format_t);
    // Convert a RISC-V instruction into a fast bytecode
    static size_t computed_index_for(format_t bits) noexcept;

		/// @brief Serializes the current CPU state to a vector
		/// @param vec The vector to serialize into
		void serialize_to(std::vector<uint8_t>& vec) const;

		/// @brief Returns the CPU to a previously stored state. Used by Machine::deserialize_from.
		/// @param vec The vector to deserialize from
		/// @param sm The serialized machine header to get metadata from
    void deserialize_from(const std::vector<uint8_t> &vec, const SerializedMachine<address_t> &sm);

    // Binary translation functions
    int load_translation(const MachineOptions<address_t> &, std::string *filename,
                         DecodedExecuteSegment<address_t> &) const;
    void try_translate(const MachineOptions<address_t> &, const std::string &,
                       std::shared_ptr<DecodedExecuteSegment<address_t>> &) const;

    void reset();
    void reset_stack_pointer() noexcept;

    CPU(Machine<address_t> &);
    CPU(Machine<address_t> &, const Machine<address_t> &other); // Fork

    DecodedExecuteSegment<address_t> &init_execute_area(const void *data, address_t begin, address_t length,
                                                        bool is_likely_jit = false);
    void set_execute_segment(DecodedExecuteSegment<address_t> &seg) noexcept { m_exec = &seg; }
    auto &current_execute_segment() noexcept { return *m_exec; }
    auto &current_execute_segment() const noexcept { return *m_exec; }
    struct NextExecuteReturn {
      DecodedExecuteSegment<address_t> *exec;
      address_t pc;
    };
    NextExecuteReturn next_execute_segment(address_t pc);
    static std::shared_ptr<DecodedExecuteSegment<address_t>> &empty_execute_segment() noexcept;
    bool is_executable(address_t addr) const noexcept;

    //-- Debugging functions --//
		/// @brief Install a breakpoint at a specific address, returning the old instruction
		uint32_t install_ebreak_at(address_t addr);

		/// @brief Install an EBREAK (breakpoint) instruction at the given address
		/// @param exec The execute segment in which to install the EBREAK instruction
		/// @param addr The address to install the EBREAK instruction
		/// @return The original instruction at the address
    static uint32_t install_ebreak_for(DecodedExecuteSegment<address_t> &exec, address_t addr);

    /// @brief Call a function for a decoder entry at a specific address
    /// @param exec The execute segment where the address is located
    /// @param addr The address to call the function for
    /// @return The decoder entry at the address
    /// @note This function allows you to retrieve the decoder entry for an executable address
    /// which can be used to modify the instruction itself and decoder data at that address.
    /// @note At the end of the call, a block-ending instruction must have been installed.
    static DecoderData<address_t> &create_block_ending_entry_at(DecodedExecuteSegment<address_t> &exec, address_t addr);

    /// @brief Modify existing function by making calls to it faster
    /// @param exec The execute segment where the address is located
    /// @param addr The address where a function already exists
    /// @return True if the fast path was created, otherwise false
    static bool create_fast_path_function(DecodedExecuteSegment<address_t> &exec, address_t addr);
    bool create_fast_path_function(address_t addr);

    // Override the function that gets called when the CPU
    // throws an execute space protection fault.
    void set_fault_handler(execute_fault_t func) noexcept { m_fault = func; }

		// Override how to produce the next active execute segment
		void set_override_new_execute_segment(override_execute_segment_t func) noexcept { m_override_exec = func; }

		// Override how to handle unknown instructions, so that you may implement your own
		static inline std::function<const instruction_t& (format_t)> on_unimplemented_instruction;
		// Retrieve default handler for unimplemented instructions (can be returned in on_unimplemented_instruction)
		static const instruction_t& get_unimplemented_instruction() noexcept;

		// Set current exception
		void set_current_exception(std::exception_ptr&& ptr) noexcept { m_current_exception = std::move(ptr); }
		void clear_current_exception() noexcept { m_current_exception = nullptr; }
		bool has_current_exception() const noexcept { return m_current_exception != nullptr; }
		auto& current_exception() const noexcept { return m_current_exception; }

	private:
    Registers<address_t> m_regs;
    Machine<address_t> &m_machine;

    // ELF programs linear .text segment (initialized as empty segment)
    DecodedExecuteSegment<address_t> *m_exec;

    // The current exception (used by eg. TCC which doesn't create unwinding tables)
    std::exception_ptr m_current_exception = nullptr;

    // The default execute fault simply triggers the exception
		execute_fault_t m_fault = [] (auto& cpu, auto&) {
			trigger_exception(EXECUTION_SPACE_PROTECTION_FAULT, cpu.pc());
		};

		// The default execute override returns no new execute segment
    override_execute_segment_t m_override_exec = [](auto &) -> DecodedExecuteSegment<address_t> & {
      return *empty_execute_segment();
    };
    static_assert((sizeof(address_t) == 4 || sizeof(address_t) == 8), "Must be either 32-bit or64-bit ISA");
};
} // namespace riscv

#include "cpu_inline.hpp"
