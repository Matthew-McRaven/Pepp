
#pragma once
#include <array>
#include <new>
#include "./events.hpp"
#include "core/ds/u64_bitset.hpp"
#include "core/integers.h"

struct EventAllocator {

  // Various performance counters for this allocater
  u8 current_allocated() const { return _slots_used.count(); }
  u64 total_allocated() const { return _counters.allocated; }
  u64 total_freed() const { return _counters.freed; }

  // Element accessors
  Event *at(Event::ID idx);
  const Event *at(Event::ID idx) const;
  auto operator[](Event::ID idx) -> Event * { return reinterpret_cast<Event *>(_slots[idx.value].data); }
  auto operator[](Event::ID idx) const -> const Event * {
    return reinterpret_cast<const Event *>(_slots[idx.value].data);
  }

  /*
   * Allocator API
   */
  // Allocate a new event in the internal buffer but do not schedule that event.
  template <EventLike DerivedEvent, typename... Args> DerivedEvent *alloc(Args... args);
  // Precondition: event has been `retired()` in the scheduler.
  void free(Event *ev);
  void free(Event::ID idx);

private:
  pepp::FixedBitset<MAX_EVENTS> _slots_used;
  alignas(alignof(EventSlot)) std::array<EventSlot, MAX_EVENTS> _slots;
  // Ensure no padding is added to event slot,
  static_assert(sizeof(_slots) == MAX_EVENTS * sizeof(EventSlot));
  mutable struct Counters {
    u64 allocated = 0; // Call count for make_event
    u64 freed = 0;     // Count for events executed and then freed
  } _counters;
};

template <EventLike DerivedEvent, typename... Args> DerivedEvent *EventAllocator::alloc(Args... args) {
  static_assert(!std::is_same_v<DerivedEvent, Event>, "Can't allocate a base event");
  // That event should be one of the supported types in our event slot.
  static_assert(EventSlot::contains<DerivedEvent>);
  // Our event slots should be properly sized and aligned for your event type.
  static_assert(sizeof(DerivedEvent) <= sizeof(_slots[0]));
  static_assert(alignof(DerivedEvent) <= alignof(EventSlot));

  if (_slots_used.count() == MAX_EVENTS) throw std::runtime_error("No free event slots available");
  size_t slot_index = _slots_used.countr_one();
  _slots_used.enable_bit(slot_index);

  EventSlot &slot = _slots[slot_index];
  // Avoiding UB by using placement new. construct_as technically has UB, but it's supposed to be "fine".
  // c++23 brings start_lifetime_as, which is the correct tool but not yet available on all platforms.
  auto ret = std::launder(new (slot.data) DerivedEvent{std::forward<Args>(args)...});
  ret->base.event_id = Event::ID(slot_index);
  _counters.allocated++;
  return ret;
}
