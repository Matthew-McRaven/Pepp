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
#include <random>
#include "core/isa/riscv/rv_base.hpp"
#include "core/isa/riscv/rv_types.hpp"
#include "notraced_riscv_isa3_system/guest_os/filedesc.hpp"
#include "notraced_riscv_isa3_system/guest_os/signals.hpp"
#include "sim3/cores/riscv/decode/decoded_exec_segment.hpp"
#include "sim3/cores/riscv/notraced_cpu.hpp"
#include "sim3/subsystems/ram/paged_pool.hpp"
#include "sim3/systems/notraced_riscv_isa3_system/rv_common.hpp"
#include "sim3/systems/notraced_riscv_isa3_system/threads.hpp"
#include "sim3/utils/auxvec.hpp"
#ifdef __cpp_exceptions
#include "notraced_riscv_isa3_system/guest_datatypes.hpp"
#endif

/**
 * Some default implementations of OS-specific I/O routines
 * stdout: Used by write/writev system calls
 * stdin:  Used by read/readv system calls
 * rdtime: Used by the RDTIME/RDTIMEH instructions
 **/
extern "C" {
#ifdef _WIN32
#include <io.h>
#else
ssize_t write(int fd, const void *buf, size_t count);
#endif
}

// #define VERBOSE_NATSYS
#ifdef VERBOSE_NATSYS
#define HPRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#define MPRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define HPRINT(fmt, ...) /* */
#define MPRINT(fmt, ...) /* */
#endif

namespace riscv
{
using RISCV32 = uint32_t; /* 32-bits CPU */
using RISCV64 = uint64_t; /* 64-bits CPU */

/// Machine is a RISC-V emulator. The W template parameter is
/// used to determine the bit-architecture, like so:
/// 32-bit:  Machine<RISCV32>, 64-bit:  Machine<RISCV64>
/// 128-bit: Machine<RISCV128>
///
/// It is instantiated with an ELF binary that contains the
/// loaded RISC-V program to run:
///
///  std::vector<uint8_t> mybinary = load_file("riscv_program.elf");
///  Machine<RISCV64> machine { mybinary };
///  machine.setup_linux_syscalls();
///  machine.setup_linux({"program", "arg0"}, {"LC_ALL=C"});
///
/// @brief A RISC-V emulator
/// @tparam W The machine architecture
template <AddressType address_t> struct alignas(RISCV_MACHINE_ALIGNMENT) Machine {
  using syscall_t = void (*)(Machine &);
  using printer_func = void (*)(const Machine &, const char *, size_t);
  using stdin_func = long (*)(const Machine &, char *, size_t);
  using rdtime_func = uint64_t (*)(const Machine &);

  /// The machine takes the binary as a const reference and does not
  /// own it, instead the binary data must be kept alive with the machine
  /// and not moved or reallocated.
  ///
  /// @brief Construct a machine with string_view pointing to a RISC-V binary
  /// @param binary The RISC-V binary that must outlive the machine
  /// @note See common.hpp for MachineOptions
  Machine(std::string_view binary, const MachineOptions<address_t> & = {});
  Machine(const std::vector<uint8_t> &binary, const MachineOptions<address_t> & = {});
#if RISCV_SPAN_AVAILABLE
		/// @brief Construct a machine with std::span pointing to a RISC-V binary
		/// @param binary The RISC-V binary that must outlive the machine
		/// @note See common.hpp for MachineOptions
		Machine(std::span<const uint8_t> binary, const MachineOptions<address_t>& = {});
#endif

		/// @brief Create an empty RISC-V machine
		/// @param opts Machine options
		Machine(const MachineOptions<address_t>& opts = {});

		/// @brief Create a thin fork from another Machine.
		/// The main machine that forks are based on must outlive its forks.
		/// @param main The machine to fork from
		/// @param opts Machine options
		///
		/// The forking constructor creates a new machine based on another,
		/// and loans all memory using Copy-on-Write mechanisms. Additionally,
		/// all cached structures like execute segment, and the instruction cache
		/// is also loaned. The main machine must not be destroyed or (in most cases)
		/// modified while the fork is running. Forks consume very little resources.
		Machine(const Machine& main, const MachineOptions<address_t>& opts = {});

		/// @brief Tears down the machine, freeing all owned memory and pages.
		~Machine();

		/// @brief Returns true if the machine has MachineOptions set.
		/// @return True if the machine has options.
		bool has_options() const noexcept { return m_options != nullptr; }
		/// @brief Set the machine options that will be used for future forked machines and execute segments.
		/// @param opts The machine options.
		void set_options(std::shared_ptr<MachineOptions<address_t>> opts) noexcept { m_options = std::move(opts); }
		/// @brief Returns the machine options that were used to create the machine.
		/// @return The machine options.
		MachineOptions<address_t>& options() const;
		MachineOptions<address_t>& options();

		/// @brief Simulate RISC-V starting from the PC register, and
		/// stopping when at most @max_instructions have been executed.
		/// If Throw == true, the machine will throw a
		/// MachineTimeoutException if it hits the provided instruction
		/// limit, and do nothing if stopped normally.
		/// @tparam Throw Throw a MachineTimeoutException if the instruction
		/// limit is reached.
		/// @param max_instructions The maximum number of instructions to
		/// execute before stopping.
		/// @param counter Set the initial instruction count.
		/// Resuming execution is equivalent to starting simulate() with
		/// the current instruction counter (this->instruction_counter()).
		/// @return Returns true if the machine stopped normally, otherwise
		/// it will return false, but only when Throw == false.
		template <bool Throw = true>
		bool simulate(uint64_t max_instructions = UINT64_MAX, uint64_t counter = 0u);

		/// @brief Resume execution, by continuing from previous PC address,
		/// with the same instruction counter, preserving its value. The only
		/// new value is the max instruction counter.
		/// @tparam Throw Throw a MachineTimeoutException if the instruction
		/// limit is reached.
		/// @param max_instructions The maximum number of instructions to
		/// execute before stopping.
		/// @return
		template <bool Throw = true>
		bool resume(uint64_t max_instructions);

		/// @brief Sets the max instructions counter to zero, which effectively
		/// causes the machine to stop. instruction_limit_reached() will return
		/// false indicating that the machine did not stop because an instruction
		/// limit was reached, and instead stopped naturally.
		void stop() noexcept;

		/// @brief Check if the machine is stopped, or in the process of stopping.
		/// This includes both when the instruction limit is reached and normal stop.
		/// This function is only relevant during execution, in for example a
		/// system call handler.
		/// @return True if the machine is stopped.
		bool stopped() const noexcept;

		/// @brief This function returns true only when simulate() ended caused
		/// by reaching the instruction limit. It will not be true if the machine
		/// stopped normally. See: machine.stopped() for that. In other words,
		/// it only returns true when execution timed out. Execution timeout is a
		/// recoverable error, and it can be used to run the emulator just a
		/// little bit now and then, making slow but sure progress.
		/// @return True if execution timed out.
		bool instruction_limit_reached() const noexcept;

		/// @brief Returns the precise number of instructions executed so far.
		/// Can be called after simulate() ends, or inside a system call handler.
		/// @return The exact number of instructions executed so far.
		uint64_t instruction_counter() const noexcept { return m_counter; }

		void     set_instruction_counter(uint64_t val) noexcept { m_counter = val; }
		void     increment_counter(uint64_t val) noexcept { m_counter += val; }
		void     reset_instruction_counter() noexcept { m_counter = 0; }
		uint64_t max_instructions() const noexcept { return m_max_counter; }
		void     set_max_instructions(uint64_t val) noexcept { m_max_counter = val; }
		/// @brief Adding a penalty is used to prevent guest programs from
		/// monopolizing the CPU by executing expensive system calls. The
		/// instruction counter is increased by the penalty value, and has
		/// the same effect as if the guest program executed that many
		/// instructions.
		/// @param val The value to add to the instruction counter.
		void     penalize(uint32_t val);

		CPU<address_t>    cpu;
		Memory<address_t> memory;

		/// @brief Copy data into the programs virtual memory, from the host.
		/// Page protections apply. Use memory.set_page_attr() to remove page
		/// protections if needed.
		/// @param dst The destination virtual address.
		/// @param buf A local buffer.
		/// @param len The size of the local buffer.
		void copy_to_guest(address_t dst, const void* buf, size_t len);

		/// @brief Copy data from the programs virtual memory, into host memory.
		/// Page protections apply. Use memory.set_page_attr() to remove page
		/// protections if needed.
		/// @param dst The destination host buffer.
		/// @param buf The virtual address to the programs buffer.
		/// @param len The size of the programs buffer.
		void copy_from_guest(void* dst, address_t buf, size_t len) const;

		/// @brief Create a startup stack for a Newlib or equivalent program.
		/// Program arguments and environment variables are pushed on the stack.
		/// Note that Newlib cannot read env variables this way.
		/// @param args An array of program main() arguments.
		/// @param env An array of program environment variables.
		void setup_argv(const std::vector<std::string>& args, const std::vector<std::string>& env = {});

		/// @brief Create a startup stack for a Linux-compatible program.
		/// Program arguments and environment variables are pushed on the stack.
		/// @param args An array of program main() arguments.
		/// @param env An array of program environment variables.
		void setup_linux(const std::vector<std::string>& args, const std::vector<std::string>& env = {});

		/// @brief Retrieve a single argument by its index for a system call.
		/// Examples: const int arg0 = machine.sysarg <int> (0);
		/// const std::string arg1 = machine.sysarg <std::string> (1);
		/// @tparam T The type of argument.
		/// @param idx The arguments index.
		/// @return The argument.
		template <typename T = address_t>
		inline T sysarg(int idx) const;

    /// @brief Retrieve a tuple of arguments based on the given types.
    /// Example: auto [str, i, f] machine.sysargs<std::string, int, float> ();
    /// Example: auto [addr, len] machine.sysargs<address_t, unsigned> ();
    /// Note: String views and riscv::Buffer consume 2 registers each.
    /// The registers consumed are the address and the length, consequtively:
    /// Example: auto [buffer] machine.sysargs<riscv::Buffer> ();
    /// Example: auto [view] machine.sysargs<std::string_view> ();
    /// @tparam ...Args A list of argument types.
    /// @return The resolved arguments in a tuple.
    template <typename... Args>
		inline auto sysargs() const;

		/// @brief Set the result of a system or function call.
		/// Only supports primitive types like integers and floats
		/// Example: machine.set_result <int, float> (123, 456.0f);
		/// NOTE: The RISC-V ABI only supports returning 2 results,
		/// using registers A0 and A1.
		/// @tparam ...Args The types of results to return.
		/// @param ...args The results to return.
		template <typename... Args>
		void set_result(Args... args) noexcept;

		/// @brief Convert the result of a C library function call that
		/// returns 0 or positive on success, and -1 on failure. errno
		/// will be returned on to the guest for negative results.
		/// @param result The result to convert to a system call return value.
		void set_result_or_error(int result);

		/// @brief A simple wrapper for getting a return value from a
		/// function call. Does not support 2-register value returns.
		/// @tparam T The type to convert the return value to.
		/// @return The result.
		template <typename T = address_t>
		inline T return_value() const { return sysarg<T> (0); }

		/// @brief Calls a RISC-V C ABI function in the program, with the
		/// provided arguments. Returns the function result.
		/// The string function name is lookup up the symbol table, which
		/// is a potentially costly and time-consuming search. It is
		/// recommended to cache the result of a function lookup using
		/// address_of(function) and instead use vmcall() with the virtual
		/// function address instead.
		/// @tparam ...Args 
		/// @tparam MAXI The instruction limit.
		/// @tparam Throw Throw exception on execution timeout.
		/// @param func_name The function to call. A symbol table lookup is performed.
		/// @param ...args The arguments to the function.
		/// @return The result of the function call.
		template <uint64_t MAXI = UINT64_MAX, bool Throw = true, typename... Args>
		constexpr address_t vmcall(const char* func_name, Args&&... args);

		/// @brief Calls a RISC-V C ABI function in the program, with the
		/// provided arguments. Returns the function result.
		/// This variant is extremely low-latency.
		/// @tparam ...Args
		/// @tparam MAXI The instruction limit.
		/// @tparam Throw Throw exception on execution timeout.
		/// @param func_addr The address of the function to call.
		/// @param ...args The arguments to the function.
		/// @return The result of the function call.
		template <uint64_t MAXI = UINT64_MAX, bool Throw = true, typename... Args>
		constexpr address_t vmcall(address_t func_addr, Args&&... args);

		/// @brief Preempt is like vmcall() except it also stores and
		/// restores the current registers and counters before and after
		/// the interrupting function call is completed. It allows calling
		/// a function as if it was an interrupt handler. The original
		/// task can be resumed again later.
		/// @tparam ...Args
		/// @tparam Throw Throw exception on execution timeout.
		/// @tparam StoreRegs Store and restore registers.
		/// @param max_instr The instruction limit, when an execution timeout happens.
		/// @param func_name The name of the function to interrupt the current task with.
		/// @param ...args
		/// @return Returns the return register from the function call.
		template<bool Throw = true, bool StoreRegs = true, typename... Args>
		address_t preempt(uint64_t max_instr, const char* func_name, Args&&... args);

		/// @brief Preempt is like vmcall() except it also stores and
		/// restores the current registers and counters before and after
		/// the interrupting function call is completed. It allows calling
		/// a function as if it was an interrupt handler. The original
		/// task can be resumed again later.
		/// @tparam ...Args 
		/// @tparam Throw Throw exception on execution timeout.
		/// @tparam StoreRegs Store and restore registers.
		/// @param max_instr The instruction limit, when an execution timeout happens.
		/// @param func_addr The address of the function to interrupt the current task with.
		/// @param ...args 
		/// @return Returns the return register from the function call.
		template<bool Throw = true, bool StoreRegs = true, typename... Args>
		address_t preempt(uint64_t max_instr, address_t func_addr, Args&&... args);

		/// @brief Performs a lookup in the symbol table and returns the address
		/// of any symbol matchine the given name. This can, for example, be
		/// used to find the address of a function.
		/// @param name The symbol to find.
		/// @return The address of the symbol, or 0x0 if not found.
		address_t address_of(std::string_view name) const;

		/// @brief Set a custom pointer that only you know the meaning of.
		/// This pointer can be retrieved from many of the callbacks in the
		/// machine, such as system calls, printers etc. It is used to
		/// facilitate wrapping the RISC-V Machine inside of your custom
		/// structure, such as a Script class.
		/// @tparam T The type of the pointer.
		/// @param data The pointer to the outer class.
		template <typename T> void set_userdata(T* data) { m_userdata = data; }

		/// @brief Return a previously set user pointer. It is usually
		/// a pointer to an outer wrapper class that manages the Machine, such
		/// as a Script class.
		/// @tparam T The type of the previously set user pointer.
		/// @return The previously set user pointer.
		template <typename T> T* get_userdata() const noexcept { return static_cast<T*> (m_userdata); }

		// Stdout, stderr (for when the guest wants to write)
		void print(const char*, size_t) const;
		auto& get_printer() const noexcept { return m_printer; }
		void set_printer(printer_func pf = default_printer) noexcept { m_printer = pf; }
		// Stdin (for when the guest wants to read)
		long stdin_read(char*, size_t) const;
		auto& get_stdin() const noexcept { return m_stdin; }
		void set_stdin(stdin_func sin = default_stdin) noexcept { m_stdin = sin; }
		// Monotonic time function (used by RDTIME and RDTIMEH)
		uint64_t rdtime() const { return m_rdtime(*this); }
		auto& get_rdtime() const noexcept { return m_rdtime; }
		void set_rdtime(rdtime_func tf = default_rdtime) noexcept { m_rdtime = tf; }

		// Push something onto the stack, moving the current stack pointer.
		address_t stack_push(const void* data, size_t length);
		address_t stack_push(const std::string& string);
		template <typename T>
		address_t stack_push(const T& pod_type);
		// Realign the stack pointer, to make sure that function calls succeed
		void realign_stack() noexcept;

		/// @brief An internal function that facilitates function
		/// calls into the guest program.
		/// @tparam ...Args 
		/// @param ...args The arguments to the function.
		template<typename... Args> constexpr
		void setup_call(Args&&... args);

		// Invoke an installed system call handler at the given index (system call number).
		void system_call(size_t);
		// Invoke the EBREAK system function
		void ebreak();

		/// @brief Install a system call handler at the given index (system call number).
		/// @param idx The system call number.
		/// @param handler The system call handler function.
    void install_syscall_handler(size_t idx, syscall_t handler);

    /// @brief Install multiple system call handlers at once.
    /// @param handlers A list of system call handlers.
    void install_syscall_handlers(std::initializer_list<std::pair<size_t, syscall_t>>);

    static void unknown_syscall_handler(Machine<address_t> &);
    constexpr auto initialize_syscalls() noexcept {
      std::array<syscall_t, RISCV_SYSCALLS_MAX> arr;
      for (auto &h : arr) h = unknown_syscall_handler;
      return arr;
    }
    // A fixed-size array of system call handlers
    std::array<syscall_t, RISCV_SYSCALLS_MAX> syscall_handlers = initialize_syscalls();
    // Callback for unimplemented system calls (default: see machine.cpp)
    static void default_unknown_syscall_no(Machine &, size_t);
    static inline void (*on_unhandled_syscall)(Machine &, size_t) = default_unknown_syscall_no;

    // Execute CSRs and system functions
    void system(union rv32i_instruction);
    // User callback for unhandled CSRs
    static inline void (*on_unhandled_csr) (Machine&, int, int, int)
			= [] (Machine<address_t>&, int, int, int) {};

		// Returns true if this machine is forked from another, and thus
		// dependent on the original machine to function properly.
		bool is_forked() const noexcept { return memory.is_forked(); }

		/// @brief Check if the performance is accelerated by a binary translator.
		/// @return True if the current execute segment is binary translated.
		bool is_binary_translation_enabled() const noexcept { return cpu.current_execute_segment().is_binary_translated(); }

		/// @brief Check if the machine has enforced and loaded an execute-only program.
		/// @return True if all execute segments are execute-only.
		bool is_execute_only() const noexcept { if (!has_options()) return false; else return options().enforce_exec_only; }

		// Optional custom native-performance arena
		bool has_arena() const noexcept { return m_arena != nullptr; }
		const Arena& arena() const;
		Arena& arena();
		void setup_native_heap(size_t sysnum, uint64_t addr, size_t size);
		void transfer_arena_from(const Machine& other);
		// Optional custom memory-related system calls
    void setup_native_memory(size_t sysnum);

    // System calls, files and threads implementations
    bool has_file_descriptors() const noexcept { return m_fds != nullptr; }
		// The "minimum": lseek, read, write, exit (provided for example usage)
    void setup_minimal_syscalls();
    // Enough to run minimal newlib programs
    void setup_newlib_syscalls();                // no filesystem access
    void setup_newlib_syscalls(bool filesystem); // optional filesystem access
    // Set up every supported system call, emulating Linux
    void setup_linux_syscalls(bool filesystem = true, bool sockets = true);
    void setup_posix_threads();
		void setup_native_threads(const size_t syscall_base);
		// Globally register a system call that clobbers all registers
    void register_clobbering_syscall(size_t sysnum);
    bool is_clobbering_syscall(size_t sysnum) noexcept;
    // Threads: Access to thread internal structures
    const MultiThreading<address_t> &threads() const;
    MultiThreading<address_t>& threads();
		bool has_threads() const noexcept { return this->m_mt != nullptr; }
		int gettid() const noexcept;
		// FileDescriptors: Access to translation between guest fds
		// and real system fds. The destructor also closes all opened files.
		const FileDescriptors& fds() const;
		FileDescriptors& fds();
		// Signal structure, lazily created
    Signals<address_t> &_signals();
    SignalAction<address_t> &sigaction(int sig) { return _signals().get(sig); }

    // Resets the machine to the initial state. It is, however, not a
		// reliable way to reset complex machines with all kinds of features
		// attached to it, and should almost never be used. It is recommended
		// to create a new machine instead, or rely on forking to facilitate
		// quickly creating and destroying a machine.
		void reset();

		/// @brief Serializes the current machine state into a vector
		/// @param vec The vector to serialize into (append)
		/// @return Returns the total number of serialized bytes
		size_t serialize_to(std::vector<uint8_t>& vec) const;

		/// @brief Returns the machine to a previously stored state
		/// NOTE: All previous memory traps are lost, syscall handlers,
		/// destructor callbacks are kept. Page fault handler and
		/// symbol lookup cache is also kept. Returns 0 on success.
		/// @param vec The vector to deserialize from
		/// @return Returns 0 on success, otherwise a non-zero integer
		int deserialize_from(const std::vector<uint8_t>& vec);

		std::pair<uint64_t&, uint64_t&> get_counters() noexcept { return {m_counter, m_max_counter}; }
		template <bool Throw = true>
		bool simulate_with(uint64_t max_instructions, uint64_t counter, address_t pc);
	private:
		template<typename... Args, std::size_t... indices>
		auto resolve_args(std::index_sequence<indices...>) const;
    void setup_native_heap_internal(const size_t);
    [[noreturn]] void timeout_exception(uint64_t);

		uint64_t     m_counter = 0;
		uint64_t     m_max_counter = 0;
		mutable void*        m_userdata = nullptr;
		mutable printer_func m_printer = default_printer;
		mutable stdin_func   m_stdin = default_stdin;
		mutable rdtime_func  m_rdtime = default_rdtime;
		std::unique_ptr<Arena> m_arena;
		std::unique_ptr<MultiThreading<address_t>> m_mt = nullptr;
		std::unique_ptr<FileDescriptors> m_fds = nullptr;
		std::unique_ptr<Signals<address_t>> m_signals = nullptr;
		std::shared_ptr<MachineOptions<address_t>> m_options = nullptr;

    static_assert((sizeof(address_t) == 4 || sizeof(address_t) == 8), "Must be either 32-bit or 64-bit ISA");
    static void default_printer(const Machine &, const char *, size_t);
    static long default_stdin(const Machine &, char *, size_t);
    static uint64_t default_rdtime(const Machine&);
};
} // namespace riscv

#include "notraced_riscv_isa3_system/inline.hpp"
