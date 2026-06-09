// See https://simpy.readthedocs.io/en/latest/examples/carwash.html

#include <algorithm>
#include <cstdio>
#include <exception>
#include <ios>
#include <random>
#include <span>
#include <variant>
#include <vector>
#include "./circq.h"
#include "core/ds/hash/djb.hpp"
#include "core/integers.h"
#include "fmt/base.h"

#include <coroutine>
#include <queue>
using pepp::djb;

struct EventLoop {
  enum class EventType : u8 { MemoryRead, Delay };
  struct Event {
    EventType type;
    void *awaitable = nullptr;
    u64 ready_at;
    std::coroutine_handle<> handle;
    auto operator<=>(const Event &o) const { return ready_at <=> o.ready_at; }
  };

  u64 current_tick = 0;
  std::priority_queue<Event, std::vector<Event>, std::greater<Event>> pending;

  static constexpr int MAX_PENDING = 8;
  std::array<Event, MAX_PENDING> pending_buf;
  int pending_size = 0;
  void push_pending(Event e) {
    // Insert sorted — linear search but n is tiny, stays in cache
    int i = pending_size++;
    pending_buf[i] = e;
    if (pending_buf[i].ready_at < pending_buf[0].ready_at) std::swap(pending_buf[i], pending_buf[0]);

    /*while (i > 0 && pending_buf[i - 1].ready_at > e.ready_at) {
      pending_buf[i] = pending_buf[i - 1];
      i--;
    }
    pending_buf[i] = e;*/
    // pending.push(e);
  }

  Event pop_pending() {
    // Min is at front (sorted ascending)
    Event e = pending_buf[0];

    // Only keep the top-1 sorted, since we only care about the next event to pop.
    if (pending_size-- > 1) {
      pending_buf[0] = pending_buf[pending_size];
      std::partial_sort(pending_buf.begin(), pending_buf.begin() + 1, pending_buf.begin() + pending_size);
    }
    return e;
    // std::shift_left(pending_buf.begin(), pending_buf.begin() + pending_size--, 1);
  }

  [[clang::noinline]] bool pending_empty() const { return pending_size == 0; }

  void run();
};

struct MemoryAwaitable {
  EventLoop &loop;
  struct AddrInfo {
    u32 address;
    u32 length = 1;
  } address;
  u64 value;

  bool await_ready() {
    if (loop.pending_empty()) {
      loop.current_tick += 1;       // memory read takes 1 cycle
      value = djb(address.address); // Dummy value for demonstration, in real implementation this would not be ready
                                    // until memory read completes.
      return true;
    } else return false;
  }
  void await_suspend(std::coroutine_handle<> handle) {
    loop.push_pending(EventLoop::Event{.type = EventLoop::EventType::MemoryRead,
                                       .awaitable = this,
                                       .ready_at = loop.current_tick + 1,
                                       .handle = handle});
  }
  template <std::integral I> I as_value() const { return static_cast<I>(value); }

  u64 await_resume() { return value; }
};

struct DelayAwaitable {
  EventLoop &loop;
  u64 cycles;

  bool await_ready() {
    if (loop.pending_empty()) {
      loop.current_tick += cycles;
      return true;
    }
    return false;
  }

  void await_suspend(std::coroutine_handle<> handle) {
    loop.push_pending(EventLoop::Event{
        .type = EventLoop::EventType::Delay, .ready_at = loop.current_tick + cycles, .handle = handle});
  }

  void await_resume() {} // nothing to return
};

struct Task {
  std::coroutine_handle<> handle; // store the handle
  struct promise_type {
    Task get_return_object() { return Task{std::coroutine_handle<promise_type>::from_promise(*this)}; }
    std::suspend_always initial_suspend() { return {}; } // start immediately
    std::suspend_never final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() { std::terminate(); }
  };
};

DelayAwaitable delay(EventLoop &loop, u64 cycles) { return DelayAwaitable{.loop = loop, .cycles = cycles}; }

void EventLoop::run() {
  while (!pending_empty()) {
    auto event = pop_pending();
    switch (event.type) {
    case EventType::MemoryRead:
      // Simulate memory read by setting value in the awaitable.
      // In a real implementation, this would involve accessing the simulator's memory.
      // For demonstration, we just set a dummy value based on the current tick.
      {
        auto awaitable = reinterpret_cast<MemoryAwaitable *>(event.awaitable);
        awaitable->value = djb(awaitable->address.address); // Dummy value
      }
      break;
    case EventType::Delay: break;
    }
    current_tick = event.ready_at;
    event.handle.resume();
    // if (event.handle.done()) event.handle.destroy();
  }
}

struct Simulator {
  i16 regs[8];
  bool nzvc[4];
  u16 pc = 0;
  int id = 0;
  volatile i64 icount = 0;
  i64 wcount = 0;
  template <typename I> MemoryAwaitable read_memory(u32 address, EventLoop &loop) {
    // fmt::println("{:04d} read_memory request for address {:08x}", loop.current_tick, address);
    MemoryAwaitable::AddrInfo info{.address = address, .length = sizeof(I)};
    return MemoryAwaitable{.loop = loop, .address = info};
  }
  [[clang::noinline]] Task execute(int maxi, EventLoop &loop) {
    while (icount < maxi) {
      // fmt::println("{:04d}[{}] Intsr begin", loop.current_tick, id);
      u8 mn = (u8) co_await read_memory<u8>(pc, loop);

      // fmt::println("{:04d}[{}] op fetched", loop.current_tick, id);
      pc++;
      if (mn < 0x80) {
        // fmt::println("{:04d}[{}] unary execute", loop.current_tick, id);
        //    co_await execute_unary(loop, mn);
        co_await delay(loop, 2); // unary takes 2 cycles
        wcount += mn;
      } else {
        // fmt::println("{:04d}[{}] opspec fetching", loop.current_tick, id);
        u16 operand = (u16) co_await read_memory<u16>(pc, loop);
        pc += 2;
        // fmt::println("{:04d}[{}] opspec fetched", loop.current_tick, id);
        // fmt::println("{:04d}[{}] nonunary execute", loop.current_tick, id);
        co_await delay(loop, 4); // nonunary takes 2 cycles
        wcount += operand << mn;
      }
      icount = icount + 1;
    }
  }
};

struct SimulatorFast {
  i16 regs[8];
  bool nzvc[4];
  u16 pc = 0;
  volatile i64 icount = 0;
  i64 current_tick = 0;
  i64 wcount = 0;
  template <typename I> I read_memory(u32 address) {
    // fmt::println("{:04d} read_memory request for address {:08x}", loop.current_tick, address);
    current_tick += 1;
    return djb(address); // Dummy value
  }
  [[clang::noinline]] void execute(int maxi) {
    while (icount < maxi) {
      // fmt::println("{:04d} Intsr begin", loop.current_tick);
      u8 mn = (u8)read_memory<u8>(pc);

      // fmt::println("{:04d} op fetched", loop.current_tick);
      pc++;
      if (mn < 0x80) {
        // fmt::println("{:04d} unary execute", loop.current_tick);
        //   co_await execute_unary(loop, mn);
        current_tick += 2;
        wcount += mn;
      } else {
        // fmt::println("{:04d} opspec fetching", loop.current_tick);
        u16 operand = (u16)read_memory<u16>(pc);
        pc += 2;
        // fmt::println("{:04d} opspec fetched", loop.current_tick);
        // fmt::println("{:04d} nonunary execute", loop.current_tick);
        current_tick += 4;
        wcount += operand << mn;
      }
      icount = icount + 1;
    }
  }
};

int main(int argc, char *argv[]) {
  int maxi = 100'000'000;
  u64 ic = 0, cc = 0, wc = 0;
  if (argc > 1 && std::string(argv[1]) == "fast") {
    SimulatorFast sim;
    sim.execute(maxi);
    ic = sim.icount, cc = sim.current_tick, wc = sim.wcount;
  } else {
    EventLoop el;
    Simulator sim;
    Simulator sim2;
    sim.id = 1, sim2.id = 2;
    auto hnd1 = sim.execute(maxi / 2, el);
    auto hnd2 = sim2.execute(maxi / 2, el);
    //  Kick off both at t=0
    el.push_pending(EventLoop::Event{.type = EventLoop::EventType::Delay, .ready_at = 0, .handle = hnd1.handle});
    el.push_pending(EventLoop::Event{.type = EventLoop::EventType::Delay, .ready_at = 0, .handle = hnd2.handle});
    el.run();
    ic = sim.icount + sim2.icount, cc = el.current_tick, wc = sim.wcount + sim2.wcount;
  }

  std::printf("Simulation finished after %lld instructions and %llu cycles\n", ic, cc);
  std::printf("Bogus wc %lld\n", wc);
  return 0;
}
