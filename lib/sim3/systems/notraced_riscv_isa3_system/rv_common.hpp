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

#include <memory>
#include <type_traits>
#include "enums/isa/rv_types.hpp"
#include "sim3/utils/function.hpp"

#if __has_include(<span>)
# include <span>
# if defined(cpp_lib_span) || defined(__cpp_lib_concepts)
#  define RISCV_SPAN_AVAILABLE 1
# endif
#endif
#include <string>
#include <string_view>
#include <vector>
#include <variant>

#ifndef RISCV_SYSCALLS_MAX
#define RISCV_SYSCALLS_MAX   512
#endif

#ifndef RISCV_SYSCALL_EBREAK_NR
#define RISCV_SYSCALL_EBREAK_NR    (RISCV_SYSCALLS_MAX-1)
#endif

#ifndef RISCV_PAGE_SIZE
#define RISCV_PAGE_SIZE  4096UL
#endif

#ifndef RISCV_FORCE_ALIGN_MEMORY
#define RISCV_FORCE_ALIGN_MEMORY 1
#endif

#ifndef RISCV_MACHINE_ALIGNMENT
#define RISCV_MACHINE_ALIGNMENT 32
#endif

#ifndef RISCV_BRK_MEMORY_SIZE
#define RISCV_BRK_MEMORY_SIZE  (16ull << 20) // 16MB
#endif

#ifndef RISCV_MAX_EXECUTE_SEGS
#define RISCV_MAX_EXECUTE_SEGS  16
#endif

namespace riscv
{
template <AddressType> struct Memory;

/// @brief Options passed to Machine constructor
/// @tparam W The RISC-V architecture
template <AddressType address_t> struct MachineOptions {
  /// @brief Maximum memory used by the machine, rounded down to
  /// the current page size (4kb).
  uint64_t memory_max = 64ull << 20; // 64MB

  /// @brief Virtual memory allocated for the main stack at construction.
  uint32_t stack_size = 1ul << 20; // 1MB default stack

  /// @brief Setting this option will load the binary at construction as if it
  /// was a RISC-V ELF binary. When disabled, no loading occurs.
  bool load_program = true;

  /// @brief Setting this option will apply page protections based on ELF segments
  /// from the program loaded at construction.
  bool protect_segments = true;

  /// @brief Enabling this will allow unsafe RWX segments (read-write-execute).
  bool allow_write_exec_segment = false;

  /// @brief Enabling this will enforce execute-only segments (X ^ R).
  bool enforce_exec_only = false;

  /// @brief Ignore .text section, as if not all executable code is in it.
  /// Instead, load all executable segments as normal. Some programs require using
  /// the .text section in order to get correctly aligned instructions.
  bool ignore_text_section = false;

  /// @brief Print some verbose loader information to stdout.
  /// @details If binary translation is enabled, this will also make the
  /// binary translation process print verbose information.
  bool verbose_loader = false;

  /// @brief Enabling this will skip assignment of copy-on-write pages
  /// to forked machines from the main machine, making fork operations faster,
  /// but requires the forks to fault in pages instead (slower).
  bool minimal_fork = false;

  /// @brief Create a linear memory arena for main memory, increasing memory
  /// locality and also enables read-write arena if the CMake option is ON.
  bool use_memory_arena = true;

  /// @brief Enable sharing of execute segments between machines.
  /// @details This will allow multiple machines to share the same execute
  /// segment, reducing memory usage and increasing performance.
  /// When binary translation is enabled, this will also share the dynamically
  /// translated code between machines. (Prevents some optimizations)
  bool use_shared_execute_segments = true;

  /// @brief Override a default-injected exit function with another function
  /// that is found by looking up the provided symbol name in the current program.
  /// Eg. if default_exit_function is "fast_exit", then the ELF binary must have
  /// that symbol visible in its .symbtab ELF section.
  std::string_view default_exit_function{};

  /// @brief Provide a custom page-fault handler at construction.
  riscv::Function<struct Page &(Memory<address_t> &, address_t, bool)> page_fault_handler = nullptr;

  /// @brief Call ebreak for each of the addresses in the vector.
  /// @details This is useful for debugging and live-patching programs.
  std::vector<std::variant<address_t, std::string>> ebreak_locations{};
};

static constexpr int SYSCALL_EBREAK = RISCV_SYSCALL_EBREAK_NR;

static constexpr size_t PageSize = RISCV_PAGE_SIZE;
static constexpr size_t PageMask = RISCV_PAGE_SIZE - 1;

#ifdef RISCV_MEMORY_TRAPS
	static constexpr bool memory_traps_enabled = true;
#else
	static constexpr bool memory_traps_enabled = false;
#endif

#if RISCV_FORCE_ALIGN_MEMORY
	static constexpr bool force_align_memory = true;
#else
	static constexpr bool force_align_memory = false;
#endif

#ifdef RISCV_DEBUG
	static constexpr bool memory_alignment_check = true;
	static constexpr bool verbose_branches_enabled = false;
	static constexpr bool unaligned_memory_slowpaths = true;
	static constexpr bool nanboxing = true;
#else
	static constexpr bool memory_alignment_check = false;
	static constexpr bool verbose_branches_enabled = false;
	static constexpr bool unaligned_memory_slowpaths = false;
#ifdef RISCV_ALWAYS_NANBOXING // In order to override the default
	static constexpr bool nanboxing = true;
#else
	static constexpr bool nanboxing = false;
#endif
#endif

#ifdef RISCV_EXT_A
#define RISCV_EXT_ATOMICS
	static constexpr bool atomics_enabled = true;
#else
	static constexpr bool atomics_enabled = false;
#endif
#ifdef RISCV_EXT_C
#define RISCV_EXT_COMPRESSED
	static constexpr bool compressed_enabled = true;
#else
	static constexpr bool compressed_enabled = false;
#endif
#define RISCV_EXT_VECTOR 32
	static constexpr unsigned vector_extension = RISCV_EXT_VECTOR;
#ifdef RISCV_FCSR
	static constexpr bool fcsr_emulation = true;
#else
	static constexpr bool fcsr_emulation = false;
#endif
	static constexpr bool binary_translation_enabled = false;
#ifdef RISCV_FLAT_RW_ARENA
	static constexpr bool flat_readwrite_arena = true;
#else
	static constexpr bool flat_readwrite_arena = false;
#endif
	static constexpr bool libtcc_enabled = false;

  template <AddressType address_t> struct MultiThreading;
  template <AddressType address_t> struct SerializedMachine;
  struct Arena;

	template <typename T>
	using remove_cvref = std::remove_cv_t<std::remove_reference_t<T>>;

	template <class...> constexpr std::false_type always_false {};

	template<typename T>
	struct is_string
		: public std::disjunction<
			std::is_same<char *, typename std::decay<T>::type>,
			std::is_same<const char *, typename std::decay<T>::type>
	> {};

	template<class T>
	struct is_stdstring : public std::is_same<T, std::basic_string<char>> {};

	template<class T>
	struct is_stdvector : public std::false_type {};

	template<class T>
	struct is_stdvector<std::vector<T>> : public std::true_type {};

	template<class T>
	struct is_stdarray_ptr : std::false_type {};

	template<class T, std::size_t N>
	struct is_stdarray_ptr<std::array<T, N>*> : std::true_type {};

	template<class T>
	constexpr bool is_stdarray_ptr_v = is_stdarray_ptr<T>::value;

	template <typename T>
	struct is_span : std::false_type{};
#ifdef RISCV_SPAN_AVAILABLE
	template <typename T>
	struct is_span<std::span<T>> : std::true_type{};
	template <typename T>
	constexpr bool is_span_v = is_span<T>::value;
#endif
} // riscv
