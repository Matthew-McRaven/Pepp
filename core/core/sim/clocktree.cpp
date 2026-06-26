#include "./clocktree.hpp"

constexpr u64 pepp::PulseSchedule::next_clock_tick(u64 tick, u8 delay_cycles) const noexcept {
  // get pulse index for current tick, increment it
  const auto idx = index_of(tick) + delay_cycles;
  return edge_time(idx);
}
