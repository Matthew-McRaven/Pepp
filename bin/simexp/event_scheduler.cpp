#include "event_scheduler.hpp"

inline u64 index_to_bitmask(u8 index) {
  if (index >= 64) [[unlikely]]
    throw std::out_of_range("Index must be less than 64");
  return 1ULL << index;
}

bool EventScheduler::skip(u64 ticks) {
  if (current_scheduled() == 0) return _current_tick += ticks, true;
  return false;
}

void EventScheduler::schedule(u8 index, u64 delay) {
  auto tick = current_tick() + delay;
  if (_scheduled[index]) [[unlikely]]
    throw std::runtime_error("Event index is already scheduled");
  //  Create a new scheduled event in-place at the tail end of the scheduled queue.
  new (&_queue[_queue_size++]) ScheduledEvent{.tick = tick, .event_index = index};
  _scheduled.enable_bit(index);
}

void EventScheduler::schedule_over(u8 dependee, u8 dependent, u64 delay) {
  if (_scheduled[dependee]) [[unlikely]]
    throw std::runtime_error("Dependee already scheduled");

  // Mark dependent as waiting on dependee
  _dependencies[dependent] |= index_to_bitmask(dependee);
  // Mark dependee as blocking dependent.
  _dependents[dependee] |= index_to_bitmask(dependent);
  _scheduled.clear_bit(dependent), _scheduled.enable_bit(dependee);

  // Check if the event is currently scheduled for execution.
  u16 idx = 0;
  for (; idx < _queue_size; idx++) {
    if (_queue[idx].event_index == dependent) break;
  }

  // The dependent is currently scheduled! Rather than pause that event and schedule a new one, just steal its spot
  if (idx != _queue_size) _queue[idx] = ScheduledEvent{.tick = _current_tick + delay, .event_index = dependee};
  // Event is not scheduled (already paused?), so we need to allocate a new spot.
  else new (&_queue[_queue_size++]) ScheduledEvent{.tick = _current_tick + delay, .event_index = dependee};
}

void EventScheduler::pause(u8 dependent, pepp::FixedBitset<MAX_EVENTS> dependees) {
  if (dependent >= MAX_EVENTS) [[unlikely]]
    throw std::out_of_range("Dependent index must be less than MAX_EVENTS");
  else if (dependees.test(dependent)) [[unlikely]] throw std::runtime_error("Event cannot depend on itself");

  _scheduled[dependent] = true;

  // Compute reverse dependencies to speed up hot code in event loop.
  _dependencies[dependent] |= dependees;
  for (u64 bits = dependees(); bits;) {
    u8 dependee = std::countr_zero(bits);
    bits &= bits - 1;
    _dependents[dependee].enable_bit(dependent);
  }

  // Check if the event is currently scheduled for execution. If so, deschedule it.
  for (u16 idx = 0; idx < _queue_size; idx++) {
    if (_queue[idx].event_index == dependent) {
      // Move last item into the hole.
      _queue[idx] = std::move(_queue[--_queue_size]);
      if (idx == 0) resort_queue();
      break;
    }
  }
}

u8 EventScheduler::next_event() {
  resort_queue();
  const auto scheduled_idx = _queue[0].event_index;
  _current_tick = _queue[0].tick, _counters.executed++;
  // If there are item left in the queue, maintain top-1 sorting requirement
  if (_queue_size-- > 1) _queue[0] = _queue[_queue_size];
  _scheduled.clear_bit(scheduled_idx);
  return scheduled_idx;
}

void EventScheduler::complete(u8 idx) {
  for (u64 bits = _dependents[idx](); bits;) {
    u8 paused_idx = std::countr_zero(bits);
    bits &= bits - 1;
    _dependencies[paused_idx].reset(idx);
    if (_dependencies[paused_idx].none()) new (&_queue[_queue_size++]) ScheduledEvent(_current_tick, paused_idx);
  }
  _dependents[idx].reset(), _dependencies[idx].reset();
}

void EventScheduler::resort_queue() {
  if (_queue_size <= 1) return; // No need to sort if we have 0 or 1 events
  auto end = _queue.begin() + _queue_size;
  auto min = std::min_element(_queue.begin(), end, [](const auto &a, const auto &b) { return a.tick < b.tick; });
  std::swap(*min, _queue[0]);
}