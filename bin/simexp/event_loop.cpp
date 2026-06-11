#include "event_loop.hpp"
#include "fmt/base.h"

EventLoop::Status EventLoop::run(u64 max_ticks) {
  return run([this, max_ticks] { return scheduler.current_tick() >= max_ticks; });
}

EventLoop::Status EventLoop::run(std::function<bool()> pause) {
  return run([this, &pause] { return pause(); });
}

void EventLoop::dump_state() const {
  fmt::println("{:04x} Current simulation state:", scheduler.current_tick());
  fmt::println("     Allocated events: {}", allocator.current_allocated());
  fmt::println("       paused: {}", paused_events());
  fmt::println("       scheduled: {}", scheduler.current_scheduled());
  fmt::println("     Event Descriptors");
  /*for (int it = 0; it < MAX_EVENTS; it++) {
    if (_event_slots_used.test(it)) {
      const auto &slot = _event_slots[it];
      const auto ev = reinterpret_cast<const Event *>(slot.data);
      switch (ev->type) {
      case Event::Type::MemoryAccess: {
        auto casted = reinterpret_cast<const MemoryRequest *>(ev);
        fmt::println("       {:02x}: type={}, source={}, address={:08x}, len={}", it, static_cast<u8>(ev->type),
                     ev->source, casted->address, casted->len);
      } break;
      default: fmt::println("       {:02x}: type={}, source={}", it, static_cast<u8>(ev->type), ev->source);
      }
    }
  }
  auto used_slots = _event_slots_used;
  fmt::println("     Scheduled Events");
  for (int it = 0; it < _queue_size; it++) {
    const auto &scheduled = _event_queue[it];
    used_slots.clear_bit(scheduled.event_index);
    fmt::println("       {:02x}: tick={}, resume={}", scheduled.event_index, scheduled.tick,
                 _scheduled_events[scheduled.event_index] ? "no" : "yes");
  }
  fmt::println("     Paused Event");
  for (; used_slots.any();) {
    const auto paused_idx = used_slots.countr_one() - 1;
    used_slots.clear_bit(paused_idx);

    const auto depset = _event_dependencies[paused_idx];
    fmt::println("       {:02x}: dependencies={:x}", paused_idx, (u64)depset());
  }*/
}
