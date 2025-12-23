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
#include "./threads.hpp"
#include "enums/isa/rv_base.hpp"

/** Implementation **/

namespace riscv {
template <AddressType address_t> inline MultiThreading<address_t>::MultiThreading(Machine<address_t> &mach) : machine(mach) {
  // Best guess for default stack boundries
  const address_t base = 0x1000;
  const address_t size = mach.memory.stack_initial() - base;
  // Create the main thread
  auto it = m_threads.try_emplace(0, *this, 0, 0x0, mach.cpu.reg(REG_SP), base, size);
  m_current = &it.first->second;
}

template <AddressType address_t>
inline MultiThreading<address_t>::MultiThreading(Machine<address_t> &mach, const MultiThreading<address_t> &other)
    : machine(mach), m_thread_counter(other.m_thread_counter), m_max_threads(other.m_max_threads) {
  for (const auto &it : other.m_threads) {
    const int tid = it.first;
    m_threads.try_emplace(tid, *this, it.second);
  }
  /* Copy each suspended by pointer lookup */
  m_suspended.reserve(other.m_suspended.size());
  for (const auto *t : other.m_suspended) {
    m_suspended.push_back(get_thread(t->tid));
  }
  /* Copy each blocked by pointer lookup */
  m_blocked.reserve(other.m_blocked.size());
  for (const auto *t : other.m_blocked) {
    m_blocked.push_back(get_thread(t->tid));
  }
  /* Copy current thread */
  m_current = get_thread(other.m_current->tid);
  if (UNLIKELY(m_current == nullptr))
    throw MachineException(INVALID_PROGRAM, "Other machine had invalid multi-threading state");
}

template <AddressType address_t> inline void Thread<address_t>::resume() {
  threading.m_current = this;
  auto &m = threading.machine;
  // restore registers
  m.cpu.registers().copy_from(Registers<address_t>::Options::NoVectors, this->stored_regs);
  THPRINT(threading.machine, "Returning to tid=%d tls=0x%lX stack=0x%lX\n", this->tid,
          (long)this->stored_regs.get(REG_TP), (long)this->stored_regs.get(REG_SP));
  // this will ensure PC is executable in all cases
  m.cpu.aligned_jump(m.cpu.pc());
}

template <AddressType address_t> inline void Thread<address_t>::suspend() {
  // copy all regs except vector lanes
  this->stored_regs.copy_from(Registers<address_t>::Options::NoVectors, threading.machine.cpu.registers());
  // add to suspended (NB: can throw)
  threading.m_suspended.push_back(this);
}

template <AddressType address_t> inline void Thread<address_t>::suspend(address_t return_value) {
  this->suspend();
  // set the *future* return value for this thread
  this->stored_regs.get(REG_ARG0) = return_value;
}

template <AddressType address_t> inline void Thread<address_t>::block(uint32_t reason, uint32_t extra) {
  // copy all regs except vector lanes
  this->stored_regs.copy_from(Registers<address_t>::Options::NoVectors, threading.machine.cpu.registers());
  this->block_word = reason;
  this->block_extra = extra;
  // add to blocked (NB: can throw)
  threading.m_blocked.push_back(this);
}

template <AddressType address_t> inline void Thread<address_t>::block_return(address_t return_value, uint32_t reason, uint32_t extra) {
  this->block(reason, extra);
  // set the block reason as the next return value
  this->stored_regs.get(REG_ARG0) = return_value;
}

template <AddressType address_t> inline Thread<address_t> *MultiThreading<address_t>::get_thread() { return this->m_current; }

template <AddressType address_t> inline Thread<address_t> *MultiThreading<address_t>::get_thread(int tid) {
  auto it = m_threads.find(tid);
  if (it == m_threads.end()) return nullptr;
  return &it->second;
}

template <AddressType address_t> inline void MultiThreading<address_t>::wakeup_next() {
  // resume a waiting thread
  if (!m_suspended.empty()) {
    auto *next = m_suspended.front();
    m_suspended.erase(m_suspended.begin());
    // resume next thread
    next->resume();
  } else {
    THPRINT(machine, "No more threads to resume. Fallback to tid=0 (*ERROR*)\n");
    auto *next = get_thread(0);
    next->resume();
  }
}

template <AddressType address_t>
inline Thread<address_t>::Thread(MultiThreading<address_t> &mt, int ttid, address_t tls, address_t stack, address_t stkbase,
                         address_t stksize)
    : threading(mt), tid(ttid), stack_base(stkbase), stack_size(stksize) {
  this->stored_regs.get(REG_TP) = tls;
  this->stored_regs.get(REG_SP) = stack;
}

template <AddressType address_t>
inline Thread<address_t>::Thread(MultiThreading<address_t> &mt, const Thread &other)
    : threading(mt), tid(other.tid), stack_base(other.stack_base), stack_size(other.stack_size),
      clear_tid(other.clear_tid), block_word(other.block_word), block_extra(other.block_extra) {
  stored_regs.copy_from(Registers<address_t>::Options::NoVectors, other.stored_regs);
}

template <AddressType address_t> inline void Thread<address_t>::activate() {
  threading.m_current = this;
  auto &cpu = threading.machine.cpu;
  cpu.reg(REG_TP) = this->stored_regs.get(REG_TP);
  cpu.reg(REG_SP) = this->stored_regs.get(REG_SP);
}

template <AddressType address_t> inline bool Thread<address_t>::exit() {
  const bool exiting_myself = (threading.get_thread() == this);
  // Copy of reference to thread manager and thread ID
  auto &thr = this->threading;
  const int tid = this->tid;
  // CLONE_CHILD_CLEARTID: set userspace TID value to zero
  if (this->clear_tid) {
    THPRINT(threading.machine, "Clearing thread value for tid=%d at 0x%lX\n", this->tid, (long)this->clear_tid);
    threading.machine.memory.template write<address_t>(this->clear_tid, 0);
  }
  // Delete this thread (except main thread)
  if (tid != 0) {
    threading.erase_thread(tid);

    // Resume next thread in suspended list
    // Exiting main thread is a "process exit", so we don't wakeup_next
    if (exiting_myself) {
      thr.wakeup_next();
    }
  }

  // tid == 0: Main thread exited
  return (tid == 0);
}

template <AddressType address_t>
inline Thread<address_t> *MultiThreading<address_t>::create(int flags, address_t ctid, address_t ptid, address_t stack, address_t tls,
                                            address_t stkbase, address_t stksize) {
  if (this->m_threads.size() >= this->m_max_threads)
    throw MachineException(INVALID_PROGRAM, "Too many threads", this->m_max_threads);

  const int tid = ++this->m_thread_counter;
  auto it = m_threads.try_emplace(tid, *this, tid, tls, stack, stkbase, stksize);
  auto *thread = &it.first->second;

  // flag for write child TID
  if (flags & CHILD_SETTID) {
    machine.memory.template write<uint32_t>(ctid, thread->tid);
  }
  if (flags & PARENT_SETTID) {
    machine.memory.template write<uint32_t>(ptid, thread->tid);
  }
  if (flags & CHILD_CLEARTID) {
    thread->clear_tid = ctid;
  }

  return thread;
}

template <AddressType address_t> inline bool MultiThreading<address_t>::preempt() {
  auto *thread = get_thread();
  if (m_suspended.empty()) {
    return false;
  }
  thread->suspend();
  this->wakeup_next();
  return true;
}

template <AddressType address_t> inline bool MultiThreading<address_t>::suspend_and_yield(long result) {
  auto *thread = get_thread();
  // don't go through the ardous yielding process when alone
  if (m_suspended.empty()) {
    // set the return value for sched_yield
    machine.cpu.reg(REG_ARG0) = result;
    return false;
  }
  // suspend current thread, and return 0 when resumed
  thread->suspend(result);
  // resume some other thread
  this->wakeup_next();
  return true;
}

template <AddressType address_t> inline bool MultiThreading<address_t>::block(address_t retval, uint32_t reason, uint32_t extra) {
  auto *thread = get_thread();
  if (UNLIKELY(m_suspended.empty())) {
    // TODO: Stop the machine here?
    return false; // continue immediately?
  }
  // block thread, write reason to future return value
  thread->block_return(retval, reason, extra);
  // resume some other thread
  this->wakeup_next();
  return true;
}

template <AddressType address_t> inline bool MultiThreading<address_t>::yield_to(int tid, bool store_retval) {
  auto *thread = get_thread();
  auto *next = get_thread(tid);
  if (next == nullptr) {
    if (store_retval) machine.cpu.reg(REG_ARG0) = -1;
    return false;
  }
  if (thread == next) {
    // immediately returning back to caller
    if (store_retval) machine.cpu.reg(REG_ARG0) = 0;
    return false;
  }
  // suspend current thread
  if (store_retval) thread->suspend(0);
  else thread->suspend();
  // remove the next thread from suspension
  for (auto it = m_suspended.begin(); it != m_suspended.end(); ++it) {
    if (*it == next) {
      m_suspended.erase(it);
      break;
    }
  }
  // resume next thread
  next->resume();
  return true;
}

template <AddressType address_t> inline void MultiThreading<address_t>::unblock(int tid) {
  for (auto it = m_blocked.begin(); it != m_blocked.end();) {
    if ((*it)->tid == tid) {
      // suspend current thread
      get_thread()->suspend(0);
      // resume this thread
      (*it)->resume();
      m_blocked.erase(it);
      return;
    } else ++it;
  }
  // given thread id was not blocked
  machine.cpu.reg(REG_ARG0) = -1;
}
template <AddressType address_t> inline size_t MultiThreading<address_t>::wakeup_blocked(size_t max, uint32_t reason, uint32_t mask) {
  size_t awakened = 0;
  for (auto it = m_blocked.begin(); it != m_blocked.end() && awakened < max;) {
    // compare against block reason
    const auto bits = (*it)->block_extra;
    if ((*it)->block_word == reason && (bits == 0 || (bits & mask) != 0)) {
      // move to suspended
      m_suspended.push_back(*it);
      m_blocked.erase(it);
      awakened++;
    } else ++it;
  }
  return awakened;
}

template <AddressType address_t> inline void MultiThreading<address_t>::erase_thread(int tid) {
  auto it = m_threads.find(tid);
  assert(it != m_threads.end());
  m_threads.erase(it);
}

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
  this->install_syscall_handler(
      syscall_base + 3, [](Machine<address_t> &machine) { machine.threads().yield_to(machine.template sysarg<uint32_t>(0)); });
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
  this->install_syscall_handler(
      syscall_base + 6, [](Machine<address_t> &machine) { machine.threads().unblock(machine.template sysarg<int>(0)); });

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
