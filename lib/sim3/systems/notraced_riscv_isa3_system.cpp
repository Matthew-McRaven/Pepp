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
#include "sim3/systems/notraced_riscv_isa3_system.hpp"
#include "sim3/alloc/arena.hpp"

// Compiler might not think these are necessary, but these headers contain templates which are needed to instantiate the
// Machine.
#include "notraced_riscv_isa3_system/defaults.hpp"
#include "notraced_riscv_isa3_system/guest_os/guest_impl.hpp"
#include "notraced_riscv_isa3_system/inline.hpp"
#include "notraced_riscv_isa3_system/serialize.hpp"
#include "notraced_riscv_isa3_system/threads.hpp"
#include "notraced_riscv_isa3_system/threads_impl.hpp"
#include "notraced_riscv_isa3_system/vmcall.hpp"

namespace riscv {
// machine.cpp
template <AddressType address_t>
inline Machine<address_t>::Machine(const Machine &other, const MachineOptions<address_t> &options)
    : cpu(*this, other), memory(*this, other, options), m_arena(nullptr) {
  this->m_counter = other.m_counter;
  this->m_max_counter = other.m_max_counter;
  if (other.m_mt) {
    m_mt.reset(new MultiThreading{*this, *other.m_mt});
  }
  // TODO: transfer arena?
}

// native_threads.hpp
static const uint32_t STACK_SIZE = 256 * 1024;

template <AddressType address_t> void Machine<address_t>::setup_native_threads(const size_t syscall_base) {
  if (this->m_mt == nullptr) this->m_mt.reset(new MultiThreading<address_t>(*this));
  // Globally register a system call that clobbers all registers
  Machine<address_t>::register_clobbering_syscall(syscall_base + 0); // microclone
  Machine<address_t>::register_clobbering_syscall(syscall_base + 1); // exit
  Machine<address_t>::register_clobbering_syscall(syscall_base + 2); // yield
  Machine<address_t>::register_clobbering_syscall(syscall_base + 3); // yield_to
  Machine<address_t>::register_clobbering_syscall(syscall_base + 4); // block
  Machine<address_t>::register_clobbering_syscall(syscall_base + 5); // unblock
  Machine<address_t>::register_clobbering_syscall(syscall_base + 6); // unblock_thread
  Machine<address_t>::register_clobbering_syscall(syscall_base + 8); // clone threadcall

  // 500: microclone
  this->install_syscall_handler(syscall_base + 0, [](Machine<address_t> &machine) {
    const auto stack = (machine.template sysarg<address_t>(0) & ~0xF);
    const auto func = machine.template sysarg<address_t>(1);
    const auto tls = machine.template sysarg<address_t>(2);
    const auto flags = machine.template sysarg<uint32_t>(3);
    const auto sbase = machine.template sysarg<address_t>(4);
    const auto ssize = machine.template sysarg<address_t>(5);
    // printf(">>> clone(func=0x%lX, stack=0x%lX, tls=0x%lX, stack base=0x%lX size=0x%lX)\n",
    //		(long)func, (long)stack, (long)tls, (long)sbase, (long)ssize);
    auto *thread = machine.threads().create(CHILD_SETTID | flags, tls, 0x0, stack, tls, sbase, ssize);
    // suspend and store return value for parent: child TID
    auto *parent = machine.threads().get_thread();
    parent->suspend(thread->tid);
    // activate and setup a function call
    thread->activate();
    // NOTE: have to start at DST-4 here!!!
    machine.setup_call(tls);
    machine.cpu.jump(func - 4);
  });
  // exit
  this->install_syscall_handler(syscall_base + 1, [](Machine<address_t> &machine) {
    const int status = machine.template sysarg<int>(0);
    THPRINT(machine, ">>> Exit on tid=%d, exit status = %d\n", machine.threads().get_tid(), (int)status);
    // Exit returns true if the program ended
    if (!machine.threads().get_thread()->exit()) {
      // Should be a new thread now
      return;
    }
    machine.stop();
    machine.set_result(status);
  });
  // sched_yield
  this->install_syscall_handler(syscall_base + 2, [](Machine<address_t> &machine) {
    // begone!
    machine.threads().suspend_and_yield();
  });
  // yield_to
  this->install_syscall_handler(syscall_base + 3, [](Machine<address_t> &machine) {
    machine.threads().yield_to(machine.template sysarg<uint32_t>(0));
  });
  // block (w/reason)
  this->install_syscall_handler(syscall_base + 4, [](Machine<address_t> &machine) {
    // begone!
    if (machine.threads().block(machine.template sysarg<int>(0), 0)) return;
    // error, we didn't block
    machine.set_result(-1);
  });
  // unblock (w/reason)
  this->install_syscall_handler(syscall_base + 5, [](Machine<address_t> &machine) {
    if (!machine.threads().wakeup_blocked(64, machine.template sysarg<int>(0))) machine.set_result(-1);
  });
  // unblock thread
  this->install_syscall_handler(syscall_base + 6, [](Machine<address_t> &machine) {
    machine.threads().unblock(machine.template sysarg<int>(0));
  });

  // super fast "direct" threads
  // N+8: clone threadcall
  this->install_syscall_handler(syscall_base + 8, [](Machine<address_t> &machine) {
    const auto func = machine.sysarg(0);
    const auto fini = machine.sysarg(1);

    const auto tls = machine.arena().malloc(STACK_SIZE);
    if (UNLIKELY(tls == 0)) {
      THPRINT(machine, "Error: Thread stack allocation failed: %#x\n", tls);
      machine.set_result(-1);
      return;
    }
    const auto stack = ((tls + STACK_SIZE) & ~0xFLL);

    auto *thread = machine.threads().create(CHILD_SETTID, tls, 0x0, stack, tls, tls, STACK_SIZE);
    // suspend and store return value for parent: child TID
    auto *parent = machine.threads().get_thread();
    parent->suspend(thread->tid);
    // activate and setup a function call
    thread->activate();
    // exit into the exit function which frees the thread
    machine.cpu.reg(riscv::REG_RA) = fini;
    // geronimo!
    machine.cpu.jump(func - 4);
  });
  // N+9: exit threadcall
  this->install_syscall_handler(syscall_base + 9, [](Machine<address_t> &machine) {
    auto retval = machine.cpu.reg(riscv::REG_RETVAL);
    auto self = machine.cpu.reg(riscv::REG_TP);
    // TODO: check this return value
    machine.arena().free(self);
    // exit thread instead
    machine.threads().get_thread()->exit();
    // return value from exited thread
    machine.set_result(retval);
  });
}
} // namespace riscv
template struct riscv::Machine<uint32_t>;
template struct riscv::Machine<uint64_t>;
template struct riscv::Thread<uint32_t>;
template struct riscv::Thread<uint64_t>;
template struct riscv::MultiThreading<uint32_t>;
template struct riscv::MultiThreading<uint64_t>;

template size_t riscv::Memory<uint32_t>::serialize_to(std::vector<uint8_t> &) const;
template void riscv::Memory<uint32_t>::deserialize_from(const std::vector<uint8_t> &,
                                                        const SerializedMachine<uint32_t> &);
template size_t riscv::Memory<uint64_t>::serialize_to(std::vector<uint8_t> &) const;
template void riscv::Memory<uint64_t>::deserialize_from(const std::vector<uint8_t> &,
                                                        const SerializedMachine<uint64_t> &);
