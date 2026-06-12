#include "event_allocator.hpp"

void EventAllocator::free(Event *ev) {
  // Ensure that the event actually belongs to us.
  if (const void *address = ev; address == nullptr) [[unlikely]]
    return;
  const auto index = Event::ID((reinterpret_cast<std::byte *>(ev) - _slots[0].data) / sizeof(EventSlot));
  free(index);
}

void EventAllocator::free(Event::ID idx) {
  /*if (idx >= MAX_EVENTS) [[unlikely]]
    throw std::runtime_error("Index too high");
  else if (!_slots_used.test(idx)) [[unlikely]] throw std::runtime_error("Event slot is not currently allocated");*/
  std::destroy_at((Event *)_slots[idx.value].data);
  _counters.freed++;
  _slots_used.clear_bit(idx.value);
}

Event *EventAllocator::at(Event::ID idx) {
  if (idx.value >= MAX_EVENTS) [[unlikely]]
    throw std::out_of_range("Index must be less than MAX_EVENTS");
  else if (!_slots_used.test(idx.value)) [[unlikely]] throw std::runtime_error("Event slot is not currently allocated");
  return (*this)[idx];
}

const Event *EventAllocator::at(Event::ID idx) const {
  if (idx.value >= MAX_EVENTS) [[unlikely]]
    throw std::out_of_range("Index must be less than MAX_EVENTS");
  else if (!_slots_used.test(idx.value)) [[unlikely]] throw std::runtime_error("Event slot is not currently allocated");
  return (*this)[idx];
}
