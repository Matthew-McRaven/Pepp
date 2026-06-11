#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include "core/integers.h"
// Any time you add a new event type, you must also modify the "slot" type in DES.
struct Event {
  bool recurs = false;
  enum class Type : u8 { Invalid = 0, MemoryAccess, SequenceEvent, Clock, MAX } type = Type::Invalid;
  u8 source = 0;
  u8 event_index = 0;
};

u8 constexpr event_type_count() { return static_cast<u8>(Event::Type::MAX); }

static_assert(std::is_standard_layout_v<Event>);

// Enforce that a type has a base member as its first data member OR the type is derived from Event directly.
template <typename T>
concept EventLike = (requires(T t) {
                      { t.base } -> std::same_as<Event &>;
                    } && std::is_standard_layout_v<T> && offsetof(T, base) == 0) || std::derived_from<T, Event>;

struct MemoryRequest {
  Event base;
  enum class Kind {
    Read,
    Write,
    Clear,
  } type;

  u32 address; // initiator-side address which read is requested for
  u32 len;     // number of bytes being read; also the size of arena pointer to by buffer
  u8 *buffer;  // A pointer to some stable bytes to read/write.
};
static_assert(EventLike<MemoryRequest>);

// A no-op event which can be used to synthetically delay a dependent event.
struct SequenceEvent {
  Event base;
};
static_assert(EventLike<SequenceEvent>);

// You received a clock. Congrats.
struct ClockEvent {
  Event base;
};
static_assert(EventLike<ClockEvent>);

// Helper to ensure that the array of events can accomodate placement new with any of our event types without any
// padding. The design pattern has a shared first member (Event base) to avoid UB with unrelated types.
template <EventLike... Ts>
// We won't call your subclass's destructor, so notify users at compile time if they try to create such an event
  requires(std::is_trivially_destructible_v<Ts> && ...)
struct Slot {
  // Helper to ensure that some type T is one of the passed types.
  template <typename T> static constexpr bool contains = (std::is_same_v<T, Ts> || ...);
  static constexpr std::size_t size = std::max({sizeof(Ts)...});
  static constexpr std::size_t alignment = std::max({alignof(Ts)...});
  // Round up size to next multiple of alignment — matches what compiler does
  static constexpr std::size_t padded_size = (size + alignment - 1) & ~(alignment - 1);
  alignas(alignment) std::byte data[padded_size];
};
using EventSlot = Slot<Event, MemoryRequest, SequenceEvent, ClockEvent>;
