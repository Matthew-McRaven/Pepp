#include "simuloop.hpp"
#include "core/ds/hash/djb.hpp"

DiscreteEventSimulator::Status DiscreteEventSimulator::run(u64 max_ticks) {
  return run([this, max_ticks] { return current_tick() >= max_ticks; });
}

void DiscreteEventSimulator::handle_event(const Event *ev) {
  switch (ev->type) {
  case Event::Type::Invalid: break;
  case Event::Type::MemoryAccess: {
    auto mem_ev = reinterpret_cast<const MemoryRequest *>(ev);
    fmt::println("Handling memory request: device={}, type={}, address={:08x}, len={}", mem_ev->base.source,
                 static_cast<u8>(mem_ev->base.type), mem_ev->address, mem_ev->len);
    auto hash = pepp::djb(mem_ev->address);
    memcpy(mem_ev->buffer, (u8 *)&hash, std::min<u8>(mem_ev->len, sizeof(hash)));
  } break;
  case Event::Type::SequenceEvent: {
    // fmt::println("Handling sequence event: device={}", ev->source);
  } break;
  case Event::Type::Clock: {
    // fmt::println("Handling clock event: device={}", ev->source);
    cpu->handle_event(*this, ev);
  }
  }
}

DiscreteEventSimulator::Status DiscreteEventSimulator::run(std::function<bool()> pause) {
  while (!pause() && _queue_size > 0) {
    dump_state();
    fmt::println("{:04x} Starting evloop", current_tick());
    /*
     * 1. Determine which event should be processed next and advance _current_tick.
     */
    ScheduledEvent scheduled;
    {
      // std::lock_guard lock(_event_slots_mutex);
      scheduled = _event_queue[0];
      // If there are item left in the queue, maintain top-1 sorting requirement
      if (_queue_size-- > 1) {
        // Move the last scheduled event to fill the hole caused by popping front
        _event_queue[0] = _event_queue[_queue_size];
        resort_queue();
        // std::partial_sort(_event_queue.begin(), _event_queue.begin() + 1, _event_queue.begin() + _queue_size);
      }
      _current_tick = scheduled.tick;
      _queue_select.reset(scheduled.event_index);
    }
    _current_tick = scheduled.tick;
    const auto ev_time = scheduled.tick;
    fmt::println("{:04x} Selected event index {} for execution", ev_time, scheduled.event_index);

    /*
     * 2. Execute or resume that event.
     */
    // This event was paused due to dependencies. Resume its coroutine.
    if (scheduled.resume) {
      scheduled.resume.resume();
    } else {
      const Event *ev = reinterpret_cast<const Event *>(_event_slots[scheduled.event_index].data);
      // TODO: find the associated handler for that event and call handle_event
      handle_event(ev);
    }

    /*
     * 3. Check if the event executed to completion. If so, unmark dependencies and free the slot.
     */
    int promoted = 0;
    // std::lock_guard lock(_event_slots_mutex);
    //  This event executed to completion. Unmark any events which depend on it,
    if (!_queue_select.test(scheduled.event_index)) {

      EventMask waiters = _event_dependents[scheduled.event_index];
      _event_dependents[scheduled.event_index].clear();

      for (u64 bits = waiters(); bits;) {
        u8 paused_idx = std::countr_zero(bits);
        bits &= bits - 1;
        _event_dependencies[paused_idx].reset(scheduled.event_index);
        if (_event_dependencies[paused_idx].none()) {
          promoted++;
          const auto dest_idx = _queue_size++;
          new (&_event_queue[dest_idx]) ScheduledEvent(_current_tick, paused_idx, _paused_queue[paused_idx]);
          _queue_select.enable_bit(paused_idx);
        }
        free_event(scheduled.event_index);
      }

      if (promoted > 0) resort_queue();
    }
  }
  return {};
}

void DiscreteEventSimulator::schedule(u8 index, u64 delay) {
  auto tick = current_tick() + delay;
  // std::lock_guard lock(_event_slots_mutex);
  if (_queue_select.test(index)) [[unlikely]]
    throw std::runtime_error("Event index is already scheduled");
  // fmt::println("{:04x} Scheduling event index {} for execution on {}", current_tick(), index, tick);
  //  Create a new scheduled event in-place at the tail end of the scheduled queue.
  new (&_event_queue[_queue_size++]) ScheduledEvent{.tick = tick, .event_index = index};
  // In the unlikely event where the new event is scheduled to execute before the previously earliest event, swap it
  // to the front. This maintains the top-1 sorting invariant for the scheduled portion.
  if (_queue_size > 1 && _event_queue[_queue_size - 1].tick < _event_queue[0].tick) [[unlikely]]
    std::swap(_event_queue[0], _event_queue[_queue_size - 1]);
  _queue_select.enable_bit(index);
}

bool DiscreteEventSimulator::scheduled(u8 index) const {
  // std::lock_guard lock(_event_slots_mutex);
  return _queue_select.test(index);
}

void DiscreteEventSimulator::mark_depends(u8 dependent, EventMask dependees) {
  fmt::println("{:04x} Set dependencies of {} to {:x}", _current_tick, dependent, (u64)dependees());
  if (dependent >= MAX_EVENTS) [[unlikely]]
    throw std::out_of_range("Dependent index must be less than MAX_EVENTS");
  else if (dependees.test(dependent)) [[unlikely]] throw std::runtime_error("Event cannot depend on itself");
  for (u16 idx = 0; idx < _queue_size; idx++) {
    if (_event_queue[idx].event_index == dependent)
      throw std::runtime_error("Cannot mark dependencies for an event which is already scheduled for execution");
  }
  _event_dependencies[dependent] |= dependees;
  for (u64 bits = dependees(); bits;) {
    u8 dependee = std::countr_zero(bits);
    bits &= bits - 1;
    _event_dependents[dependee].enable_bit(dependent);
  }
}

void DiscreteEventSimulator::pause(u8 dependent, EventMask dependees, std::coroutine_handle<> resume) {
  if (dependent >= MAX_EVENTS) [[unlikely]]
    throw std::out_of_range("Dependent index must be less than MAX_EVENTS");
  else if (dependees.test(dependent)) [[unlikely]] throw std::runtime_error("Event cannot depend on itself");
  else if (resume == nullptr) [[unlikely]] throw std::invalid_argument("Resume handle cannot be null");
  fmt::println("{:04x} Pausing {} and setting dependencies to {:x}", _current_tick, dependent, (u64)dependees());

  // Compute reverse dependencies to speed up hot code in event loop.
  _event_dependencies[dependent] |= dependees;
  for (u64 bits = dependees(); bits;) {
    u8 dependee = std::countr_zero(bits);
    bits &= bits - 1;
    _event_dependents[dependee].enable_bit(dependent);
  }

  if (!_queue_select.test(dependent)) [[unlikely]]
    return;
  _queue_select.clear_bit(dependent);
  // Check if the event is currently scheduled for execution.
  u16 idx = 0;
  for (; idx < _queue_size; idx++)
    if (_event_queue[idx].event_index == dependent) break;

  // The event is currently scheduled! Move it to the paused list and update its resume handle.
  if (idx != _queue_size) {
    const auto dest_idx = _event_queue[idx].event_index;
    // Move last item into the hole.
    _event_queue[idx] = std::move(_event_queue[--_queue_size]);
    _paused_queue[dest_idx] = resume;
    if (idx == 0) resort_queue();
  } else _paused_queue[dependent] = resume;
}

void DiscreteEventSimulator::dump_state() const { lockless_dumpstate(); }

void DiscreteEventSimulator::lockless_dumpstate() const {
  fmt::println("{:04x} Current simulation state:", _current_tick);
  fmt::println("     Allocated events: {}", _event_slots_used.count());
  fmt::println("       paused: {}", MAX_EVENTS - _queue_select.count());
  fmt::println("       scheduled: {}", _queue_select.count());
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
                 scheduled.resume ? "yes" : "no");
  }
  /*fmt::println("     Paused Events ({})", _paused.count());
  for (int it = _paused_top; it < MAX_EVENTS; it++) {
    const auto &paused = _event_queue[it];
    const auto depset = _event_dependencies[paused.event_index];
    fmt::println("       {:02x}: dependencies={:x}", paused.event_index, (u64)depset());
  }*/
}

void DiscreteEventSimulator::free_event(Event *ev) {
  // Ensure that the event actually belongs to us.
  if (const void *address = ev; address == nullptr) return;
  else if (address < _event_slots[0].data || address >= _event_slots[MAX_EVENTS - 1].data + sizeof(EventSlot))
    throw std::runtime_error("Invalid event pointer");
  // Do not destroy until after we have computed its slot.
  size_t slot_index = (reinterpret_cast<std::byte *>(ev) - _event_slots[0].data) / sizeof(EventSlot);
  fmt::println("{:04x} Freeing {}", _current_tick, slot_index);
  std::destroy_at(ev);
    _event_slots_used.clear_bit(slot_index);
    _queue_select.clear_bit(slot_index);
    _event_dependencies[slot_index].clear();
    _event_dependents[slot_index].clear();
    if (_event_dependencies[slot_index].any()) [[unlikely]]
      throw std::runtime_error("Event still has dependents when freed");
}

void DiscreteEventSimulator::free_event(u8 idx) { free_event(reinterpret_cast<Event *>(_event_slots[idx].data)); }

void DiscreteEventSimulator::resort_queue() {
  if (_queue_size <= 1) return; // No need to sort if we have 0 or 1 events
  auto end = _event_queue.begin() + _queue_size;
  auto min = std::min_element(_event_queue.begin(), end, [](const auto &a, const auto &b) { return a.tick < b.tick; });
  std::swap(*min, _event_queue[0]);
}
