
#include <cstdio>
#include <string>
#include "./pep10isa.hpp"
#include "core/ds/hash/djb.hpp"
#include "core/integers.h"
#include "fmt/base.h"

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
    Simulator s;
    auto cpu = s.make_device<Pep10CPU, EventLoop &>("cpu", s.loop());
    auto dram = s.make_device<DRAM>("dram");
    s.dispatcher().register_handler(cpu->id(), Event::Type::Clock, cpu->id());
    s.dispatcher().register_handler(cpu->id(), Event::Type::MemoryAccess, dram->id());
    // auto snooper = s.dispatcher.install_filter<AccessSnooper<DRAM>>({sim.id(), Event::Type::MemoryAccess});
    // snooper->_id = 3;
    i64 *ptr = &cpu->icount;
    auto ev = s.allocator().alloc<ClockEvent>(cpu->id());
    ev->base.recurs = true;
    s.scheduler().schedule(ev->base.event_index, 0);
    s.run([ptr, maxi]() { return *ptr >= maxi; });
    ic = cpu->icount, cc = s.scheduler().current_tick(), wc = cpu->wcount;
    fmt::println("Executed {}, allocated {} and freed {} events", s.scheduler().total_executed(),
                 s.allocator().total_allocated(), s.allocator().total_freed());
    // fmt::println("Access memory {} times", snooper->access_count);
  }

  std::printf("Simulation finished after %lld instructions and %llu cycles\n", ic, cc);
  std::printf("Bogus wc %lld\n", wc);
  return 0;
}
