
#include <algorithm>
#include <cstdio>
#include <exception>
#include <ios>
#include <random>
#include <span>
#include <variant>
#include <vector>

#include "./simuloop.hpp"
#include "core/ds/hash/djb.hpp"
#include "core/integers.h"
#include "fmt/base.h"

#include <coroutine>

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
    return pepp::djb(address); // Dummy value
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
    DiscreteEventSimulator s;
    Pep10CPU sim;
    s.cpu = &sim;
    i64 *ptr = &sim.icount;
    auto ev = s.make_event<ClockEvent>();
    ev->base.type = Event::Type::Clock;
    s.schedule(ev->base.event_index, 0);
    s.run([ptr, maxi]() { return *ptr >= maxi; });
    ic = sim.icount, cc = s.current_tick(), wc = sim.wcount;
    fmt::println("Executed {}, posted {} and retired {} events", s.executed, s.posted, s.retired);
  }

  std::printf("Simulation finished after %lld instructions and %llu cycles\n", ic, cc);
  std::printf("Bogus wc %lld\n", wc);
  return 0;
}
