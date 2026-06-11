#include "event_allocator.hpp"

void EventAllocator::free(Event *ev) {
  // Ensure that the event actually belongs to us.
  if (const void *address = ev; address == nullptr) [[unlikely]]
    return;
  size_t slot_index = (reinterpret_cast<std::byte *>(ev) - _slots[0].data) / sizeof(EventSlot);
  free(slot_index);
}

void EventAllocator::free(u8 idx) {
  if (idx >= MAX_EVENTS) [[unlikely]]
    throw std::runtime_error("Index too high");
  else if (!_slots_used.test(idx)) [[unlikely]] throw std::runtime_error("Event slot is not currently allocated");
  std::destroy_at((Event *)_slots[idx].data);

  _counters.freed++;
  _slots_used.clear_bit(idx);
}

Event *EventAllocator::at(u8 idx) {
  if (idx >= MAX_EVENTS) [[unlikely]]
    throw std::out_of_range("Index must be less than MAX_EVENTS");
  else if (!_slots_used.test(idx)) [[unlikely]] throw std::runtime_error("Event slot is not currently allocated");
  return (*this)[idx];
}

const Event *EventAllocator::at(u8 idx) const {
  if (idx >= MAX_EVENTS) [[unlikely]]
    throw std::out_of_range("Index must be less than MAX_EVENTS");
  else if (!_slots_used.test(idx)) [[unlikely]] throw std::runtime_error("Event slot is not currently allocated");
  return (*this)[idx];
}
