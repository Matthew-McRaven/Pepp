#pragma once

#include <concepts>
#include <cstddef>
#include "core/integers.h"
// Any time you add a new event type, you must also modify the "slot" type in DES.
struct Event {
  enum class Type : u8 {
    Invalid = 0,
    MemoryAccess,
    SequenceEvent,
    Clock,
  } type = Type::Invalid;
  u8 source = 0;
  u8 event_index = 0;
};

static_assert(std::is_standard_layout_v<Event>);

// Enforce that a type has a base member as its first data member.
template <typename T>
concept EventLike = requires(T t) {
  { t.base } -> std::same_as<Event &>;
} && std::is_standard_layout_v<T> && offsetof(T, base) == 0;

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
