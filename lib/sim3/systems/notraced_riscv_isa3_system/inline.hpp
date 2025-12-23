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
#include "../notraced_riscv_isa3_system.hpp"

namespace riscv {

template <AddressType address_t> inline void Machine<address_t>::stop() noexcept { m_max_counter = 0; }
template <AddressType address_t> inline bool Machine<address_t>::stopped() const noexcept { return m_counter >= m_max_counter; }
template <AddressType address_t> inline bool Machine<address_t>::instruction_limit_reached() const noexcept {
  return m_counter >= m_max_counter && m_max_counter != 0;
}

template <AddressType address_t>
template <bool Throw>
inline bool Machine<address_t>::simulate_with(uint64_t max_instr, uint64_t counter, address_t pc) {
  const bool stopped_normally = cpu.simulate(pc, counter, max_instr);
  if constexpr (Throw) {
    // The simulation either ends normally, or it throws an exception
    if (UNLIKELY(!stopped_normally)) timeout_exception(max_instr);
    return true;
  } else {
    // Here m_max_counter is useful for instruction_limit_reached() and stopped().
    this->m_max_counter = stopped_normally ? 0 : max_instr;
    return stopped_normally;
  }
}

template <AddressType address_t> template <bool Throw> inline bool Machine<address_t>::simulate(uint64_t max_instr, uint64_t counter) {
  return this->simulate_with<Throw>(max_instr, counter, cpu.pc());
}

template <AddressType address_t> template <bool Throw> inline bool Machine<address_t>::resume(uint64_t max_instr) {
  return this->simulate<Throw>(this->instruction_counter() + max_instr, this->instruction_counter());
}

template <AddressType address_t> inline void Machine<address_t>::reset() {
  cpu.reset();
  memory.reset();
}

template <AddressType address_t> inline void Machine<address_t>::print(const char *buffer, size_t len) const {
  this->m_printer(*this, buffer, len);
}
template <AddressType address_t> inline long Machine<address_t>::stdin_read(char *buffer, size_t len) const {
  return this->m_stdin(*this, buffer, len);
}

template <AddressType address_t> inline void Machine<address_t>::install_syscall_handler(size_t sysn, syscall_t handler) {
  // A work-around for thread-sanitizer false positives (setting the same handler)
  if (syscall_handlers.at(sysn) != handler) syscall_handlers.at(sysn) = handler;
}
template <AddressType address_t>
inline void Machine<address_t>::install_syscall_handlers(std::initializer_list<std::pair<size_t, syscall_t>> syscalls) {
  for (auto &scall : syscalls) install_syscall_handler(scall.first, scall.second);
}

template <AddressType address_t> inline void Machine<address_t>::system_call(size_t sysnum) {
  if (LIKELY(sysnum < syscall_handlers.size())) {
    Machine::syscall_handlers[RISCV_SPECSAFE(sysnum)](*this);
  } else {
    on_unhandled_syscall(*this, sysnum);
  }
}

template <AddressType address_t> template <typename T> inline T Machine<address_t>::sysarg(int idx) const {
  if constexpr (std::is_integral_v<T>) {
    // 64-bit integers on 32-bit uses 2 registers
    if constexpr (sizeof(T) > sizeof(address_t)) {
      return static_cast<T>(cpu.reg(REG_ARG0 + idx)) | static_cast<T>(cpu.reg(REG_ARG0 + idx + 1)) << 32;
    }
    return static_cast<T>(cpu.reg(REG_ARG0 + idx));
  } else if constexpr (std::is_same_v<T, float>) return cpu.registers().getfl(REG_FA0 + idx).f32[0];
  else if constexpr (std::is_same_v<T, double>) return cpu.registers().getfl(REG_FA0 + idx).f64;
  else if constexpr (std::is_enum_v<T>) return static_cast<T>(cpu.reg(REG_ARG0 + idx));
  else if constexpr (std::is_same_v<T, riscv::Buffer>)
    return memory.membuffer(cpu.reg(REG_ARG0 + idx), cpu.reg(REG_ARG0 + idx + 1));
  else if constexpr (std::is_same_v<T, std::basic_string_view<char>>)
    return memory.memview(cpu.reg(REG_ARG0 + idx), cpu.reg(REG_ARG0 + idx + 1));
  else if constexpr (is_stdstring<T>::value) return memory.memstring(cpu.reg(REG_ARG0 + idx));
  else if constexpr (std::is_pointer_v<remove_cvref<T>>) {
    return (T)memory.template memarray<std::remove_pointer_t<std::remove_reference_t<T>>>(cpu.reg(REG_ARG0 + idx), 1);
  }
#ifdef RISCV_SPAN_AVAILABLE
  else if constexpr (is_span_v<T>)
    return memory.template memspan<typename T::value_type>(cpu.reg(REG_ARG0 + idx), cpu.reg(REG_ARG0 + idx + 1));
#endif // RISCV_SPAN_AVAILABLE
  else if constexpr (std::is_standard_layout_v<remove_cvref<T>> && std::is_trivial_v<remove_cvref<T>>) {
    T value;
    memory.memcpy_out(&value, cpu.reg(REG_ARG0 + idx), sizeof(T));
    return value;
  } else static_assert(always_false<T>, "Unknown type");
}

template <AddressType address_t>
template <typename... Args, std::size_t... Indices>
inline auto Machine<address_t>::resolve_args(std::index_sequence<Indices...>) const {
  std::tuple<std::decay_t<Args>...> retval;
  size_t i = 0;
  size_t f = 0;
  (
      [&] {
        if constexpr (std::is_integral_v<Args>) {
          std::get<Indices>(retval) = sysarg<Args>(i++);
          if constexpr (sizeof(Args) > sizeof(address_t)) i++; // uses 2 registers
        } else if constexpr (std::is_floating_point_v<Args>) std::get<Indices>(retval) = sysarg<Args>(f++);
        else if constexpr (std::is_enum_v<Args>) std::get<Indices>(retval) = sysarg<Args>(i++);
        else if constexpr (std::is_same_v<Args, riscv::Buffer>) {
          std::get<Indices>(retval) = std::move(sysarg<Args>(i));
          i += 2; // ptr, len
        } else if constexpr (std::is_same_v<Args, std::basic_string_view<char>>) {
          std::get<Indices>(retval) = sysarg<Args>(i);
          i += 2;
        } else if constexpr (is_stdstring<Args>::value) std::get<Indices>(retval) = sysarg<Args>(i++);
        else if constexpr (is_stdarray_ptr_v<Args>)
          std::get<Indices>(retval) = sysarg<Args>(i++); // Fixed: One register
#ifdef RISCV_SPAN_AVAILABLE
        else if constexpr (is_span_v<Args>) {
          std::get<Indices>(retval) = sysarg<Args>(i);
          i += 2; // Dynamic: Two registers
        }
#endif // RISCV_SPAN_AVAILABLE
        else if constexpr (std::is_standard_layout_v<remove_cvref<Args>> && std::is_trivial_v<remove_cvref<Args>>)
          std::get<Indices>(retval) = sysarg<Args>(i++);
        else static_assert(always_false<Args>, "Unknown type");
      }(),
      ...);
  return retval;
}

template <AddressType address_t> template <typename... Args> inline auto Machine<address_t>::sysargs() const {
  return resolve_args<Args...>(std::index_sequence_for<Args...>{});
}

template <AddressType address_t> template <typename... Args> inline void Machine<address_t>::set_result(Args... args) noexcept {
  size_t i = 0;
  size_t f = 0;
  (
      [&] {
        if constexpr (std::is_integral_v<Args>) {
          if constexpr (sizeof(Args) < sizeof(address_t) && !std::is_same_v<Args, bool>)
            // Sign-extend all arguments smaller than the word size
            cpu.registers().get(REG_ARG0 + i++) = (typename std::make_signed_t<Args>)args;
          else cpu.registers().get(REG_ARG0 + i++) = args;
        } else if constexpr (std::is_enum_v<Args>) cpu.registers().get(REG_ARG0 + i++) = static_cast<int>(args);
        else if constexpr (std::is_same_v<Args, float>) cpu.registers().getfl(REG_FA0 + f++).set_float(args);
        else if constexpr (std::is_same_v<Args, double>) cpu.registers().getfl(REG_FA0 + f++).set_double(args);
        else static_assert(always_false<Args>, "Unknown type");
      }(),
      ...);
}

template <AddressType address_t> inline void Machine<address_t>::ebreak() {
  // its simpler and more flexible to just call a user-provided function
  this->system_call(riscv::SYSCALL_EBREAK);
}

template <AddressType address_t> inline void Machine<address_t>::copy_to_guest(address_t dst, const void *buf, size_t len) {
  memory.memcpy(dst, buf, len);
}

template <AddressType address_t> inline void Machine<address_t>::copy_from_guest(void *dst, address_t buf, size_t len) const {
  memory.memcpy_out(dst, buf, len);
}

template <AddressType address_t> inline address_t Machine<address_t>::address_of(std::string_view name) const {
  return memory.resolve_address(name);
}

template <AddressType address_t> address_t Machine<address_t>::stack_push(const void *data, size_t length) {
  auto &sp = cpu.reg(REG_SP);
  sp = (sp - length) & ~(address_t)(sizeof(address_t) - 1); // maintain word alignment
  this->copy_to_guest(sp, data, length);
  return sp;
}
template <AddressType address_t> inline address_t Machine<address_t>::stack_push(const std::string &string) {
  return stack_push(string.data(), string.size() + 1); /* zero */
}
template <AddressType address_t> template <typename T> inline address_t Machine<address_t>::stack_push(const T &type) {
  static_assert(std::is_standard_layout_v<T>, "Must be a POD type");
  return stack_push(&type, sizeof(T));
}

template <AddressType address_t> inline void Machine<address_t>::realign_stack() noexcept {
  // the RISC-V calling convention mandates a 16-byte alignment
  cpu.reg(REG_SP) &= ~address_t{0xF};
}

template <AddressType address_t> inline const MultiThreading<address_t> &Machine<address_t>::threads() const {
  if (LIKELY(m_mt != nullptr)) return *m_mt;
#if __cpp_exceptions
  throw MachineException(FEATURE_DISABLED, "Threads are not initialized");
#else
  std::abort();
#endif
}
template <AddressType address_t> inline MultiThreading<address_t> &Machine<address_t>::threads() {
  if (LIKELY(m_mt != nullptr)) return *m_mt;
#if __cpp_exceptions
  throw MachineException(FEATURE_DISABLED, "Threads are not initialized");
#else
  std::abort();
#endif
}

template <AddressType address_t> inline const FileDescriptors &Machine<address_t>::fds() const {
  if (m_fds != nullptr) return *m_fds;
#if __cpp_exceptions
  throw MachineException(ILLEGAL_OPERATION, "No access to files or sockets", 0);
#else
  std::abort();
#endif
}
template <AddressType address_t> inline FileDescriptors &Machine<address_t>::fds() {
  if (m_fds != nullptr) return *m_fds;
#if __cpp_exceptions
  throw MachineException(ILLEGAL_OPERATION, "No access to files or sockets", 0);
#else
  std::abort();
#endif
}

template <AddressType address_t> inline Signals<address_t> &Machine<address_t>::_signals() {
  if (m_signals == nullptr) m_signals.reset(new Signals<address_t>);
  return *m_signals;
}

template <AddressType address_t> inline MachineOptions<address_t> &Machine<address_t>::options() const {
  if (m_options == nullptr)
#if __cpp_exceptions
    throw MachineException(ILLEGAL_OPERATION, "Machine options have not been set/initialized");
#else
    std::abort();
#endif
  return *m_options;
}
template <AddressType address_t> inline MachineOptions<address_t> &Machine<address_t>::options() {
  if (m_options == nullptr)
#if __cpp_exceptions
    throw MachineException(ILLEGAL_OPERATION, "Machine options have not been set/initialized");
#else
    std::abort();
#endif
  return *m_options;
}

// machine.cpp
template <AddressType address_t>
inline Machine<address_t>::Machine(std::string_view binary, const MachineOptions<address_t> &options)
    : cpu(*this), memory(*this, binary, options), m_arena(nullptr) {
  cpu.reset();
}

template <AddressType address_t>
inline Machine<address_t>::Machine(const std::vector<uint8_t> &bin, const MachineOptions<address_t> &opts)
    : Machine(std::string_view{(const char *)bin.data(), bin.size()}, opts) {}

#if RISCV_SPAN_AVAILABLE
template <AddressType address_t>
inline Machine<address_t>::Machine(std::span<const uint8_t> binary, const MachineOptions<address_t> &options)
    : Machine(std::string_view{(const char *)binary.data(), binary.size()}, options) {}
#endif

template <AddressType address_t> inline Machine<address_t>::Machine(const MachineOptions<address_t> &opts) : Machine(std::string_view{}, opts) {}

template <AddressType address_t> Machine<address_t>::~Machine() {}

template <AddressType address_t> void Machine<address_t>::unknown_syscall_handler(Machine<address_t> &machine) {
  const auto syscall_number = machine.cpu.reg(REG_ECALL);
  machine.on_unhandled_syscall(machine, syscall_number);
}

template <AddressType address_t> void Machine<address_t>::default_unknown_syscall_no(Machine<address_t> &machine, size_t num) {
  auto txt = "Unhandled system call: " + std::to_string(num) + "\n";
  machine.print(txt.c_str(), txt.size());
}

template <AddressType address_t> void Machine<address_t>::register_clobbering_syscall(size_t) {}

template <AddressType address_t> bool Machine<address_t>::is_clobbering_syscall(size_t) noexcept {
  return false; // No clobbering syscalls in non-binary translation mode
}

template <AddressType address_t> void Machine<address_t>::set_result_or_error(int result) {
  if (result >= 0) set_result(result);
  else set_result(-errno);
}

template <AddressType address_t> void Machine<address_t>::penalize(uint32_t val) { m_counter += val; }

template <AddressType address_t> PEPP_COLD_PATH() void Machine<address_t>::timeout_exception(uint64_t max_instr) {
  throw MachineTimeoutException(MAX_INSTRUCTIONS_REACHED, "Instruction count limit reached", max_instr);
}

template <AddressType address_t>
void Machine<address_t>::setup_argv(const std::vector<std::string> &args, const std::vector<std::string> &env) {
  // Arguments to main()
  std::vector<address_t> argv;
  argv.push_back(args.size()); // argc
  for (const auto &string : args) {
    const auto sp = stack_push(string);
    argv.push_back(sp);
  }
  argv.push_back(0x0);
  for (const auto &string : env) {
    const auto sp = stack_push(string);
    argv.push_back(sp);
  }
  argv.push_back(0x0);

  // Extra aligned SP and copy the arguments over
  auto &sp = cpu.reg(REG_SP);
  const size_t argsize = argv.size() * sizeof(argv[0]);
  sp -= argsize;
  sp &= ~(address_t)0xF; // mandated 16-byte stack alignment

  this->copy_to_guest(sp, argv.data(), argsize);
}

template <AddressType address_t, typename T> const T *elf_offset(riscv::Machine<address_t> &machine, intptr_t ofs) {
  return (const T *)&machine.memory.binary().at(ofs);
}
template <AddressType address_t> inline const auto *elf_header(riscv::Machine<address_t> &machine) {
  return elf_offset<address_t, typename riscv::Elf<address_t>::Header>(machine, 0);
}

template <AddressType address_t>
static inline void push_arg(Machine<address_t> &m, std::vector<address_t> &vec, address_t &dst,
                            const std::string &str) {
  const size_t size = str.size() + 1;
  dst -= size;
  dst &= ~(address_t)(sizeof(address_t) - 1); // maintain alignment
  vec.push_back(dst);
  m.copy_to_guest(dst, str.data(), size);
}
template <AddressType address_t> static inline void push_aux(std::vector<address_t> &vec, AuxVec<address_t> aux) {
  vec.push_back(aux.a_type);
  vec.push_back(aux.a_val);
}
template <AddressType address_t>
static inline void push_down(Machine<address_t> &m, address_t &dst, const void *data, size_t size) {
  dst -= size;
  dst &= ~(address_t)(sizeof(address_t) - 1); // maintain alignment
  m.copy_to_guest(dst, data, size);
}

template <AddressType address_t>
void Machine<address_t>::setup_linux(const std::vector<std::string> &args, const std::vector<std::string> &env) {
#if defined(__linux__) && !defined(RISCV_DISABLE_URANDOM)
  static std::random_device rd("/dev/urandom");
#else
  static std::random_device rd{};
#endif
  // start installing at near-end of address space, leaving room on both sides
  // stack below and installation above
  auto dst = this->cpu.reg(REG_SP);

  // inception :)
  std::uniform_int_distribution<int> rand(0, 256);

  std::array<uint8_t, 16> canary;
  std::generate(canary.begin(), canary.end(), [&] { return rand(rd); });
  push_down(*this, dst, canary.data(), canary.size());
  const auto canary_addr = dst;

  const char *platform = (sizeof(address_t) == 4) ? "RISC-V 32-bit" : "RISC-V 64-bit";
  push_down(*this, dst, platform, strlen(platform) + 1);
  const auto platform_addr = dst;

  // Program headers
  const auto *binary_ehdr = elf_header<address_t>(*this);
  const auto *binary_phdr = elf_offset<address_t, typename Elf<address_t>::ProgramHeader>(*this, binary_ehdr->e_phoff);
  const int phdr_count = int(binary_ehdr->e_phnum);
  // Check if we have a PT_PHDR program header already loaded into memory
  address_t phdr_location = 0;
  for (int i = 0; i < phdr_count; i++) {
    if (binary_phdr[i].p_type == Elf<address_t>::PT_PHDR) {
      phdr_location = this->memory.elf_base_address(binary_phdr[i].p_vaddr);
      break;
    }
  }
  if (phdr_location == 0) {
    for (int i = phdr_count - 1; i >= 0; i--) {
      const auto *phd = &binary_phdr[i];
      push_down(*this, dst, phd, sizeof(typename Elf<address_t>::ProgramHeader));
    }
    phdr_location = dst;
  } else {
    // Verify that the PT_PHDR is loaded at the correct address
    if (memory.memcmp(binary_phdr, phdr_location, phdr_count * sizeof(*binary_phdr)) != 0) {
      throw MachineException(INVALID_PROGRAM, "PT_PHDR program header is not loaded at the correct address");
    }
  }

  // Arguments to main()
  std::vector<address_t> argv;
  argv.push_back(args.size()); // argc
  for (const auto &string : args) {
    push_arg(*this, argv, dst, string);
  }
  argv.push_back(0x0);

  // Environment vars
  for (const auto &string : env) {
    push_arg(*this, argv, dst, string);
  }
  argv.push_back(0x0);

  // Auxiliary vector
  push_aux<address_t>(argv, {AT_PAGESZ, Page::size()});
  push_aux<address_t>(argv, {AT_CLKTCK, 100});

  // ELF related
  push_aux<address_t>(argv, {AT_PHDR, phdr_location});
  push_aux<address_t>(argv, {AT_PHENT, sizeof(*binary_phdr)});
  push_aux<address_t>(argv, {AT_PHNUM, unsigned(phdr_count)});

  // Misc
  push_aux<address_t>(argv, {AT_BASE, address_t(this->memory.start_address() & ~0xFFFFFFLL)});
  push_aux<address_t>(argv, {AT_ENTRY, this->memory.start_address()});
  push_aux<address_t>(argv, {AT_HWCAP, 0});
  push_aux<address_t>(argv, {AT_HWCAP2, 0});
  push_aux<address_t>(argv, {AT_UID, 1000});
  push_aux<address_t>(argv, {AT_EUID, 0});
  push_aux<address_t>(argv, {AT_GID, 0});
  push_aux<address_t>(argv, {AT_EGID, 0});
  push_aux<address_t>(argv, {AT_SECURE, 0});

  push_aux<address_t>(argv, {AT_PLATFORM, platform_addr});

  // supplemental randomness
  push_aux<address_t>(argv, {AT_RANDOM, canary_addr});
  push_aux<address_t>(argv, {AT_NULL, 0});

  // from this point on the stack is starting, pointing @ argc
  // install the arg vector
  const size_t argsize = argv.size() * sizeof(argv[0]);
  dst -= argsize;
  dst &= ~0xFLL; // mandated 16-byte stack alignment
  this->copy_to_guest(dst, argv.data(), argsize);
  // re-initialize machine stack-pointer
  this->cpu.reg(REG_SP) = dst;
}

template <AddressType address_t> void Machine<address_t>::system(union rv32i_instruction instr) {
  switch (instr.Itype.funct3) {
  case 0x0: // SYSTEM functions
    switch (instr.Itype.imm) {
    case 0: // ECALL
      this->system_call(cpu.reg(REG_ECALL));
      return;
    case 1: // EBREAK
      this->ebreak();
      return;
    case 0x105: // WFI
      this->stop();
      return;
    case 0x7FF: // Stop machine
      this->stop();
      return;
    }
    break;
  case 0x1: { // CSRRW: Atomically swap CSR and integer register
    const bool rd = instr.Itype.rd != 0;
    switch (instr.Itype.imm) {
    case 0x001: // fflags: accrued exceptions
      if (rd) cpu.reg(instr.Itype.rd) = cpu.registers().fcsr().fflags;
      cpu.registers().fcsr().fflags = cpu.reg(instr.Itype.rs1);
      return;
    case 0x002: // frm: rounding-mode
      if (rd) cpu.reg(instr.Itype.rd) = cpu.registers().fcsr().frm;
      cpu.registers().fcsr().frm = cpu.reg(instr.Itype.rs1);
      return;
    case 0x003: // fcsr: control and status register
      if (rd) cpu.reg(instr.Itype.rd) = cpu.registers().fcsr().whole;
      cpu.registers().fcsr().whole = cpu.reg(instr.Itype.rs1) & 0xFF;
      return;
    }
    [[fallthrough]];
  }
  case 0x2: { // CSRRS: Atomically read and set bit mask
    // if destination is x0, then we do not write to rd
    const bool rd = instr.Itype.rd != 0;
    switch (instr.Itype.imm) {
    case 0x001: // fflags (accrued exceptions)
      if (rd) cpu.reg(instr.Itype.rd) = cpu.registers().fcsr().fflags;
      cpu.registers().fcsr().fflags |= cpu.reg(instr.Itype.rs1);
      return;
    case 0x002: // frm (rounding-mode)
      if (rd) cpu.reg(instr.Itype.rd) = cpu.registers().fcsr().frm;
      cpu.registers().fcsr().frm |= cpu.reg(instr.Itype.rs1);
      return;
    case 0x003: // fcsr (control and status register)
      if (rd) cpu.reg(instr.Itype.rd) = cpu.registers().fcsr().whole;
      cpu.registers().fcsr().whole |= cpu.reg(instr.Itype.rs1) & 0xFF;
      return;
    case 0xC00: // CSR RDCYCLE (lower)
    case 0xC02: // RDINSTRET (lower)
      if (rd) {
        cpu.reg(instr.Itype.rd) = this->instruction_counter();
        return;
      } else {
        if (instr.Itype.rs1 == 0) // UNIMP instruction
          cpu.trigger_exception(UNIMPLEMENTED_INSTRUCTION, instr.Itype.imm);
        else // CYCLE is not writable
          cpu.trigger_exception(ILLEGAL_OPERATION, instr.Itype.imm);
      }
    case 0xC80: // CSR RDCYCLE (upper)
    case 0xC82: // RDINSTRET (upper)
      if (rd) cpu.reg(instr.Itype.rd) = this->instruction_counter() >> 32u;
      return;
    case 0xC01: // CSR RDTIME (lower)
      if (rd) cpu.reg(instr.Itype.rd) = m_rdtime(*this);
      return;
    case 0xC81: // CSR RDTIME (upper)
      if (rd) cpu.reg(instr.Itype.rd) = m_rdtime(*this) >> 32u;
      return;
    case 0xF11: // CSR marchid
      if (rd) cpu.reg(instr.Itype.rd) = 0;
      return;
    case 0xF12: // CSR mvendorid
      if (rd) cpu.reg(instr.Itype.rd) = 0;
      return;
    case 0xF13: // CSR mimpid
      if (rd) cpu.reg(instr.Itype.rd) = 1;
      return;
    case 0xF14: // CSR mhartid
      if (rd) cpu.reg(instr.Itype.rd) = cpu.cpu_id();
      return;
    default: on_unhandled_csr(*this, instr.Itype.imm, instr.Itype.rd, instr.Itype.rs1); return;
    }
  } break;
  case 0x3: { // CSRRC: Atomically read and clear CSR
    const bool rd = instr.Itype.rd != 0;
    switch (instr.Itype.imm) {
    case 0x001: // fflags: accrued exceptions
      if (rd) cpu.reg(instr.Itype.rd) = cpu.registers().fcsr().fflags;
      cpu.registers().fcsr().fflags &= ~cpu.reg(instr.Itype.rs1);
      return;
    case 0x002: // frm: rounding-mode
      if (rd) cpu.reg(instr.Itype.rd) = cpu.registers().fcsr().frm;
      cpu.registers().fcsr().frm &= ~cpu.reg(instr.Itype.rs1);
      return;
    case 0x003: // fcsr: control and status register
      if (rd) cpu.reg(instr.Itype.rd) = cpu.registers().fcsr().whole;
      cpu.registers().fcsr().whole &= ~(cpu.reg(instr.Itype.rs1) & 0xFF);
      return;
    }
    break;
  }
  case 0x5: { // CSRWI: CSRW from uimm[4:0] in RS1
    const bool rd = instr.Itype.rd != 0;
    const uint32_t imm = instr.Itype.rs1;
    switch (instr.Itype.imm) {
    case 0x001: // fflags: accrued exceptions
      if (rd) cpu.reg(instr.Itype.rd) = cpu.registers().fcsr().fflags;
      cpu.registers().fcsr().fflags = imm;
      return;
    case 0x002: // frm: rounding-mode
      if (rd) cpu.reg(instr.Itype.rd) = cpu.registers().fcsr().frm;
      cpu.registers().fcsr().frm = imm;
      return;
    case 0x003: // fcsr: control and status register
      if (rd) cpu.reg(instr.Itype.rd) = cpu.registers().fcsr().whole;
      cpu.registers().fcsr().whole = imm & 0xFF;
      return;
    default: on_unhandled_csr(*this, instr.Itype.imm, instr.Itype.rd, instr.Itype.rs1); return;
    }
  } // CSRWI
  case 0x7: { // CSRRCI: Atomically read and clear CSR using immediate
    const bool rd = instr.Itype.rd != 0;
    const uint32_t imm = instr.Itype.rs1;
    switch (instr.Itype.imm) {
    case 0x001: // fflags: accrued exceptions
      if (rd) cpu.reg(instr.Itype.rd) = cpu.registers().fcsr().fflags;
      cpu.registers().fcsr().fflags &= ~imm;
      return;
    case 0x002: // frm: rounding-mode
      if (rd) cpu.reg(instr.Itype.rd) = cpu.registers().fcsr().frm;
      cpu.registers().fcsr().frm &= ~imm;
      return;
    case 0x003: // fcsr: control and status register
      if (rd) cpu.reg(instr.Itype.rd) = cpu.registers().fcsr().whole;
      cpu.registers().fcsr().whole &= ~(imm & 0xFF);
      return;
    default: on_unhandled_csr(*this, instr.Itype.imm, instr.Itype.rd, instr.Itype.rs1); return;
    }
    break;
  } // CSRRCI
  }
  // if we got here, its an illegal operation!
  cpu.trigger_exception(ILLEGAL_OPERATION, instr.Itype.funct3);
}

// posix/signals.cpp

// native_libc.cpp
// An arbitrary maximum length just to stop *somewhere*
static constexpr uint64_t MEMCPY_MAX = 1024ull * 1024u * 512u; // 512M
static constexpr size_t MEMCPY_BUFFERS = 256u;                 /* 1MB of maximally fragmented memory */
static constexpr uint32_t STRLEN_MAX = 64'000u;
static constexpr uint64_t COMPLEX_CALL_PENALTY = 2'000u;

template <AddressType address_t> void Machine<address_t>::setup_native_heap_internal(const size_t syscall_base) {
  // Malloc n+0
  Machine<address_t>::install_syscall_handler(syscall_base + 0, [](Machine<address_t> &machine) {
    const size_t len = machine.sysarg(0);
    auto data = machine.arena().malloc(len);
    HPRINT("SYSCALL malloc(%zu) = 0x%lX\n", len, (long)data);
    machine.set_result(data);
    machine.penalize(COMPLEX_CALL_PENALTY);
  });
  // Calloc n+1
  Machine<address_t>::install_syscall_handler(syscall_base + 1, [](Machine<address_t> &machine) {
    const auto [count, size] = machine.template sysargs<address_t, address_t>();
    const size_t len = count * size;
    auto data = machine.arena().malloc(len);
    HPRINT("SYSCALL calloc(%zu, %zu) = 0x%lX\n", (size_t)count, (size_t)size, (long)data);
    if (data != 0) {
      // XXX: Not using memzero as it has known issues
      machine.memory.memset(data, 0, len);
      machine.penalize(len);
    }
    machine.set_result(data);
    machine.penalize(COMPLEX_CALL_PENALTY);
  });
  // Realloc n+2
  Machine<address_t>::install_syscall_handler(syscall_base + 2, [](Machine<address_t> &machine) {
    const auto src = machine.sysarg(0);
    const auto newlen = machine.sysarg(1);

    const auto [data, srclen] = machine.arena().realloc(src, newlen);
    HPRINT("SYSCALL realloc(0x%lX:%zu, %zu) = 0x%lX\n", (long)src, (size_t)srclen, (size_t)newlen, (long)data);
    // When data != src, srclen is the old length, and the
    // chunks are non-overlapping, so we can use forwards memcpy.
    if (data != src && srclen != 0) {
      machine.memory.memcpy(data, machine, src, std::min(address_t(srclen), newlen));
      machine.penalize(2 * srclen);
    }
    machine.set_result(data);
    machine.penalize(COMPLEX_CALL_PENALTY);
  });
  // Free n+3
  Machine<address_t>::install_syscall_handler(syscall_base + 3, [](Machine<address_t> &machine) {
    const auto ptr = machine.sysarg(0);
    if (ptr != 0x0) {
      [[maybe_unused]] int ret = machine.arena().free(ptr);
      HPRINT("SYSCALL free(0x%lX) = %d\n", (long)ptr, ret);
      // machine.set_result(ret);
      if (ret < 0) {
        throw MachineException(SYSTEM_CALL_FAILED, "Possible double-free for freed pointer", ptr);
      }
      machine.penalize(COMPLEX_CALL_PENALTY);
      return;
    }
    HPRINT("SYSCALL free(0x0) = 0\n");
    // machine.set_result(0);
    machine.penalize(COMPLEX_CALL_PENALTY);
    return;
  });
  // Meminfo n+4
  Machine<address_t>::install_syscall_handler(syscall_base + 4, [](Machine<address_t> &machine) {
    const auto dst = machine.sysarg(0);
    const auto &arena = machine.arena();
    struct Result {
      const address_t bf;
      const address_t bu;
      const address_t cu;
    } result = {
        .bf = (address_t)arena.bytes_free(), .bu = (address_t)arena.bytes_used(), .cu = (address_t)arena.chunks_used()};
    int ret = (dst != 0) ? 0 : -1;
    HPRINT("SYSCALL meminfo(0x%lX) = %d\n", (long)dst, ret);
    if (ret == 0) {
      machine.copy_to_guest(dst, &result, sizeof(result));
    }
    machine.set_result(ret);
    machine.penalize(COMPLEX_CALL_PENALTY);
  });
}

template <AddressType address_t> const Arena &Machine<address_t>::arena() const {
  if (UNLIKELY(m_arena == nullptr)) throw MachineException(SYSTEM_CALL_FAILED, "Arena not created on this machine");
  return *m_arena;
}
template <AddressType address_t> Arena &Machine<address_t>::arena() {
  if (UNLIKELY(m_arena == nullptr)) throw MachineException(SYSTEM_CALL_FAILED, "Arena not created on this machine");
  return *m_arena;
}
template <AddressType address_t> void Machine<address_t>::setup_native_heap(size_t sysnum, uint64_t base, size_t max_memory) {
  m_arena.reset(new Arena(base, base + max_memory));

  this->setup_native_heap_internal(sysnum);
}
template <AddressType address_t> void Machine<address_t>::transfer_arena_from(const Machine &other) { m_arena.reset(new Arena(other.arena())); }

template <AddressType address_t> void Machine<address_t>::setup_native_memory(const size_t syscall_base) {
  Machine<address_t>::install_syscall_handlers(
      {{syscall_base + 0,
        [](Machine<address_t> &m) {
          // Memcpy n+0
          auto [dst, src, len] = m.sysargs<address_t, address_t, address_t>();
          MPRINT("SYSCALL memcpy(%#lX, %#lX, %zu)\n", (long)dst, (long)src, (size_t)len);
          m.memory.memcpy(dst, m, src, len);
          m.penalize(2 * len);
        }},
       {syscall_base + 1,
        [](Machine<address_t> &m) {
          // Memset n+1
          const auto [dst, value, len] = m.sysargs<address_t, int, address_t>();
          MPRINT("SYSCALL memset(%#lX, %#X, %zu)\n", (long)dst, value, (size_t)len);
          if (UNLIKELY(len > MEMCPY_MAX)) throw MachineException(SYSTEM_CALL_FAILED, "memset length too large", len);
          m.memory.memset(dst, value, len);
          m.penalize(len);
        }},
       {syscall_base + 2,
        [](Machine<address_t> &m) {
          // Memmove n+2
          auto [dst, src, len] = m.sysargs<address_t, address_t, address_t>();
          MPRINT("SYSCALL memmove(%#lX, %#lX, %zu)\n", (long)dst, (long)src, (size_t)len);
          // If we have a flat readwrite arena, we can use memmove
          if constexpr (riscv::flat_readwrite_arena) {
            if (m.memory.try_memmove(dst, src, len)) {
              m.penalize(2 * len);
              return;
            }
          }
          // If the buffers don't overlap, we can use memcpy which copies forwards
          if (dst < src) {
            std::array<riscv::vBuffer, MEMCPY_BUFFERS> buffers;
            const size_t cnt = m.memory.gather_buffers_from_range(buffers.size(), buffers.data(), src, len);
            for (size_t i = 0; i < cnt; i++) {
              m.memory.memcpy(dst, buffers[i].ptr, buffers[i].len);
              dst += buffers[i].len;
            }
          } else if (len > 0) {
            if (UNLIKELY(len > MEMCPY_MAX)) throw MachineException(SYSTEM_CALL_FAILED, "memmove length too large", len);
            constexpr size_t wordsize = sizeof(address_t);
            if (dst % wordsize == 0 && src % wordsize == 0 && len % wordsize == 0) {
              // Copy whole registers backwards
              // We start at len because unsigned doesn't have negative numbers
              // so we will have to read and write from index i-1 instead.
              for (unsigned i = len; i != 0; i -= wordsize) {
                m.memory.template write<address_t>(dst + i - wordsize,
                                                   m.memory.template read<address_t>(src + i - wordsize));
              }
            } else {
              // Copy byte by byte backwards
              for (unsigned i = len; i != 0; i--) {
                m.memory.template write<uint8_t>(dst + i - 1, m.memory.template read<uint8_t>(src + i - 1));
              }
            }
          }
          m.penalize(2 * len);
        }},
       {syscall_base + 3,
        [](Machine<address_t> &m) {
          // Memcmp n+3
          auto [p1, p2, len] = m.sysargs<address_t, address_t, address_t>();
          MPRINT("SYSCALL memcmp(%#lX, %#lX, %zu)\n", (long)p1, (long)p2, (size_t)len);
          if (UNLIKELY(len > MEMCPY_MAX)) throw MachineException(SYSTEM_CALL_FAILED, "memcmp length too large", len);
          m.penalize(2 * len);
          m.set_result(m.memory.memcmp(p1, p2, len));
        }},
       {syscall_base + 5,
        [](Machine<address_t> &m) {
          // Strlen n+5
          auto [addr] = m.sysargs<address_t>();
          uint32_t len = m.memory.strlen(addr, STRLEN_MAX);
          m.penalize(2 * len);
          m.set_result(len);
          MPRINT("SYSCALL strlen(%#lX) = %u\n", (long)addr, len);
        }},
       {syscall_base + 6,
        [](Machine<address_t> &m) {
          // Strncmp n+6
          auto [a1, a2, maxlen] = m.sysargs<address_t, address_t, uint32_t>();
          MPRINT("SYSCALL strncmp(%#lX, %#lX, %u)\n", (long)a1, (long)a2, maxlen);
          maxlen = std::min(maxlen, STRLEN_MAX);
          uint32_t len = 0;
          while (len < maxlen) {
            const uint8_t v1 = m.memory.template read<uint8_t>(a1++);
            const uint8_t v2 = m.memory.template read<uint8_t>(a2++);
            if (v1 != v2 || v1 == 0) {
              m.penalize(2 + 2 * len);
              m.set_result(v1 - v2);
              return;
            }
            len++;
          }
          m.penalize(2 + 2 * len);
          m.set_result(0);
        }},
       {syscall_base + 13,
        [](Machine<address_t> &m) {
          // Reserved system call n+13
          // Space for one more accelerated libc function
          m.set_result(-1);
        }},
       {syscall_base + 14, [](Machine<address_t> &m) {
          // Print backtrace n+14
          m.memory.print_backtrace([&](std::string_view line) {
            m.print(line.data(), line.size());
            m.print("\n", 1);
          });
          m.set_result(0);
          m.penalize(100 * COMPLEX_CALL_PENALTY);
        }}});
}
} // namespace riscv

#include "./threads.hpp"
#include "./vmcall.hpp"
