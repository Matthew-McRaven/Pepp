#include "simuloop.hpp"
#include <algorithm>
#include <fmt/format.h>
#include "./pep10isa.hpp"
#include "core/ds/hash/djb.hpp"

EventLoop::Status EventLoop::run(u64 max_ticks) {
  return run([this, max_ticks] { return current_tick() >= max_ticks; });
}

EventLoop::Status EventLoop::run(std::function<bool()> pause) {
  return run([this, &pause] { return pause(); });
}

void EventLoop::handle_event(const Event *ev) {
  switch (ev->type) {
  case Event::Type::Invalid: break;
  case Event::Type::MemoryAccess: {
    auto mem_ev = reinterpret_cast<const MemoryRequest *>(ev);
    auto hash = pepp::djb(mem_ev->address);
    memcpy(mem_ev->buffer, (u8 *)&hash, std::min<u8>(mem_ev->len, sizeof(hash)));
  } break;
  case Event::Type::SequenceEvent: break;
  case Event::Type::Clock: cpu->handle_event(*this, ev);
  }
}

bool EventLoop::skip(u64 ticks) {
  if (_queue_size == 0) {
    return _current_tick += ticks, true;
  }
  return false;
}

u64 EventLoop::current_tick() const noexcept { return _current_tick; }

void EventLoop::schedule(u8 index, u64 delay) {
  auto tick = current_tick() + delay;
  if (_coro_slots[index]) [[unlikely]]
    throw std::runtime_error("Event index is already scheduled");
  //  Create a new scheduled event in-place at the tail end of the scheduled queue.
  new (&_event_queue[_queue_size++]) ScheduledEvent{.tick = tick, .event_index = index};
  // In the unlikely event where the new event is scheduled to execute before the previously earliest event, swap it
  // to the front. This maintains the top-1 sorting invariant for the scheduled portion.
  if (_queue_size > 1 && _event_queue[_queue_size - 1].tick < _event_queue[0].tick) [[unlikely]]
    std::swap(_event_queue[0], _event_queue[_queue_size - 1]);
}

void EventLoop::schedule_over(u8 dependee, u8 dependent, std::coroutine_handle<> resume, u64 delay) {
  if (_coro_slots[dependee]) [[unlikely]]
    throw std::runtime_error("Dependee already scheduled");

  // Mark dependent as waiting on dependee
  _event_dependencies[dependent] |= index_to_bitmask(dependee);
  _coro_slots[dependent] = resume;
  // Mark dependee as blocking dependent.
  _event_dependents[dependee] |= index_to_bitmask(dependent);

  // Check if the event is currently scheduled for execution.
  u16 idx = 0;
  for (; idx < _queue_size; idx++) {
    if (_event_queue[idx].event_index == dependent) {
      break;
    }
  }

  // The dependent is currently scheduled! Rather than pause that event and schedule a new one, just steal its spot
  if (idx != _queue_size) _event_queue[idx] = ScheduledEvent{.tick = _current_tick + delay, .event_index = dependee};
  // Event is not scheduled (already paused?), so we need to allocate a new spot.
  else new (&_event_queue[_queue_size++]) ScheduledEvent{.tick = _current_tick + delay, .event_index = dependee};
  // While I might be able to optimize the placement algorithm to avoid resort, this is a sane default for now until
  // profiling proves otherwise.
  if (_queue_size > 1) resort_queue();
}

bool EventLoop::scheduled(u8 index) const {
  // std::lock_guard lock(_event_slots_mutex);
  return _coro_slots[index] == nullptr;
}

void EventLoop::pause(u8 dependent, EventMask dependees, std::coroutine_handle<> resume) {
  if (dependent >= MAX_EVENTS) [[unlikely]]
    throw std::out_of_range("Dependent index must be less than MAX_EVENTS");
  else if (dependees.test(dependent)) [[unlikely]] throw std::runtime_error("Event cannot depend on itself");
  else if (resume == nullptr) [[unlikely]] throw std::invalid_argument("Resume handle cannot be null");

  _coro_slots[dependent] = resume;

  // Compute reverse dependencies to speed up hot code in event loop.
  _event_dependencies[dependent] |= dependees;
  for (u64 bits = dependees(); bits;) {
    u8 dependee = std::countr_zero(bits);
    bits &= bits - 1;
    _event_dependents[dependee].enable_bit(dependent);
  }

  // Check if the event is currently scheduled for execution. If so, deschedule it.
  for (u16 idx = 0; idx < _queue_size; idx++) {
    if (_event_queue[idx].event_index == dependent) {
      // Move last item into the hole.
      _event_queue[idx] = std::move(_event_queue[--_queue_size]);
      if (idx == 0) resort_queue();
      break;
    }
  }
}

void EventLoop::dump_state() const {
  fmt::println("{:04x} Current simulation state:", _current_tick);
  fmt::println("     Allocated events: {}", _event_slots_used.count());
  // fmt::println("       paused: {}", MAX_EVENTS - _queue_select.count());
  // fmt::println("       scheduled: {}", _queue_select.count());
  fmt::println("     Event Descriptors");
  for (int it = 0; it < MAX_EVENTS; it++) {
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
  fmt::println("     Scheduled Events ({})", _queue_size);
  for (int it = 0; it < _queue_size; it++) {
    const auto &scheduled = _event_queue[it];
    fmt::println("       {:02x}: tick={}, resume={}", scheduled.event_index, scheduled.tick,
                 _coro_slots[scheduled.event_index] ? "yes" : "no");
  }
  /*fmt::println("     Paused Events ({})", _paused.count());
  for (int it = _paused_top; it < MAX_EVENTS; it++) {
    const auto &paused = _event_queue[it];
    const auto depset = _event_dependencies[paused.event_index];
    fmt::println("       {:02x}: dependencies={:x}", paused.event_index, (u64)depset());
  }*/
}

void EventLoop::free_event(Event *ev) {
  // Ensure that the event actually belongs to us.
  if (const void *address = ev; address == nullptr) [[unlikely]]
    return;
  else if (address < _event_slots[0].data || address >= _event_slots[MAX_EVENTS - 1].data + sizeof(EventSlot))
      [[unlikely]]
    throw std::runtime_error("Invalid event pointer");
  // Do not destroy until after we have computed its slot.
  size_t slot_index = (reinterpret_cast<std::byte *>(ev) - _event_slots[0].data) / sizeof(EventSlot);
  std::destroy_at(ev);

  _counters.freed++;
  _coro_slots[slot_index] = nullptr;
  _event_dependencies[slot_index].clear(), _event_dependents[slot_index].clear();
  _event_slots_used.clear_bit(slot_index);
  if (_event_dependencies[slot_index].any()) [[unlikely]]
    throw std::runtime_error("Event still has dependents when freed");
}

void EventLoop::free_event(u8 idx) { free_event(reinterpret_cast<Event *>(_event_slots[idx].data)); }

void EventLoop::resort_queue() {
  if (_queue_size <= 1) return; // No need to sort if we have 0 or 1 events
  auto end = _event_queue.begin() + _queue_size;
  auto min = std::min_element(_event_queue.begin(), end, [](const auto &a, const auto &b) { return a.tick < b.tick; });
  std::swap(*min, _event_queue[0]);
}
