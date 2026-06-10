#include "simuloop.hpp"
#include <algorithm>
#include <fmt/format.h>

EventLoop::Status EventLoop::run(u64 max_ticks) {
  return run([this, max_ticks] { return current_tick() >= max_ticks; });
}

EventLoop::Status EventLoop::run(std::function<bool()> pause) {
  return run([this, &pause] { return pause(); });
}

void EventLoop::register_device(u8 source, EventHandler *handler) {
  const auto size = source + 1;
  if (devices.size() < size) devices.resize(size, nullptr);
  devices[source] = handler;
}

u16 combine(u8 source, Event::Type type) {
  return static_cast<u16>(source) * event_type_count() + static_cast<u8>(type);
}

void EventLoop::register_handler(u8 source, Event::Type ev, u8 handler) {
  const auto size = source + 1;
  if (handlers.size() < size * event_type_count()) handlers.resize(size * event_type_count(), 0);
  handlers[combine(source, ev)] = handler;
}

void EventLoop::handle_event(const Event *ev) {
  EventHandler *hnd = nullptr;
  const auto target = handlers[combine(ev->source, ev->type)];
  if (target == 0 || (hnd = devices[target]) == nullptr) [[unlikely]] {
    throw std::runtime_error(
        fmt::format("No handler registered for event type {} from source {}", static_cast<u8>(ev->type), ev->source));
  } else hnd->handle_event(ev);
}

bool EventLoop::skip(u64 ticks) {
  if (_queue_size == 0) return _current_tick += ticks, true;
  return false;
}

u64 EventLoop::current_tick() const noexcept { return _current_tick; }

void EventLoop::schedule(u8 index, u64 delay) {
  auto tick = current_tick() + delay;
  if (_scheduled_events[index]) [[unlikely]]
    throw std::runtime_error("Event index is already scheduled");
  //  Create a new scheduled event in-place at the tail end of the scheduled queue.
  new (&_event_queue[_queue_size++]) ScheduledEvent{.tick = tick, .event_index = index};
  _scheduled_events.enable_bit(index);
}

void EventLoop::schedule_over(u8 dependee, u8 dependent, u64 delay) {
  if (_scheduled_events[dependee]) [[unlikely]]
    throw std::runtime_error("Dependee already scheduled");

  // Mark dependent as waiting on dependee
  _event_dependencies[dependent] |= index_to_bitmask(dependee);
  _scheduled_events[dependent] = false;
  // Mark dependee as blocking dependent.
  _event_dependents[dependee] |= index_to_bitmask(dependent);
  _scheduled_events.enable_bit(dependee);

  // Check if the event is currently scheduled for execution.
  u16 idx = 0;
  for (; idx < _queue_size; idx++) {
    if (_event_queue[idx].event_index == dependent) break;
  }

  // The dependent is currently scheduled! Rather than pause that event and schedule a new one, just steal its spot
  if (idx != _queue_size) _event_queue[idx] = ScheduledEvent{.tick = _current_tick + delay, .event_index = dependee};
  // Event is not scheduled (already paused?), so we need to allocate a new spot.
  else new (&_event_queue[_queue_size++]) ScheduledEvent{.tick = _current_tick + delay, .event_index = dependee};
}

bool EventLoop::scheduled(u8 index) const { return _scheduled_events.test(index); }

void EventLoop::pause(u8 dependent, EventMask dependees) {
  if (dependent >= MAX_EVENTS) [[unlikely]]
    throw std::out_of_range("Dependent index must be less than MAX_EVENTS");
  else if (dependees.test(dependent)) [[unlikely]] throw std::runtime_error("Event cannot depend on itself");

  _scheduled_events[dependent] = true;

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
  fmt::println("       paused: {}", paused_events());
  fmt::println("       scheduled: {}", _queue_size);
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
  }
}

void EventLoop::free_event(Event *ev) {
  // Ensure that the event actually belongs to us.
  if (const void *address = ev; address == nullptr) [[unlikely]]
    return;
  size_t slot_index = (reinterpret_cast<std::byte *>(ev) - _event_slots[0].data) / sizeof(EventSlot);
  free_event(slot_index);
}

void EventLoop::free_event(u8 idx) {
  if (idx >= MAX_EVENTS) [[unlikely]]
    throw std::runtime_error("Index too high");
  else if (!_event_slots_used.test(idx)) [[unlikely]] throw std::runtime_error("Event slot is not currently allocated");
  else if (_event_dependencies[idx].any()) [[unlikely]] throw std::runtime_error("Event still has dependencies");
  std::destroy_at((Event *)_event_slots[idx].data);

  _counters.freed++;
  _scheduled_events.clear_bit(idx);
  _event_dependencies[idx].clear(), _event_dependents[idx].clear();
  _event_slots_used.clear_bit(idx);
}

void EventLoop::resort_queue() {
  if (_queue_size <= 1) return; // No need to sort if we have 0 or 1 events
  auto end = _event_queue.begin() + _queue_size;
  auto min = std::min_element(_event_queue.begin(), end, [](const auto &a, const auto &b) { return a.tick < b.tick; });
  std::swap(*min, _event_queue[0]);
}
