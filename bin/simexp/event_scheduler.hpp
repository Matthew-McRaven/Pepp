#pragma once
#include <array>
#include "./events.hpp"
#include "core/ds/u64_bitset.hpp"
#include "core/integers.h"
struct EventScheduler {
  // Performance counters
  u64 current_tick() const noexcept { return _current_tick; }
  u8 current_scheduled() const noexcept { return _queue_size; }
  u64 total_scheduled() const noexcept { return _counters.scheduled; }
  u64 total_executed() const noexcept { return _counters.executed; }

  // If no scheduled events are pending, advance the clock by the given ticks and return true. Else, return false
  // without modifying current_tick();
  bool skip(u64 ticks);

  // Returns true if the event index is currently scheduled for execution.
  bool scheduled(u8 index) const;
  // Take the index of an allocated event and schedule it to run after a given tick delay.
  void schedule(u8 index, u64 delay = 0);
  // Mark dependent as paused on dependee, and schedule dependee for execution with a delay.
  // More efficient than a pause() followed by a schedule()
  void schedule_over(u8 dependee, u8 dependent, u64 delay);
  // Remove dependent event from the schedule until all events in dependees have executed, at which point dependent is
  // re-scheduled for execution. If resume is not a nullptr, that coroutine will be executed rather the the original
  // event handler.
  void pause(u8 dependent, pepp::FixedBitset<MAX_EVENTS> dependees);

  // Pop the top element from the queue and update current tick. Return value is the index of the event to be handled.
  u8 pop_front();
  // Mark all dependees of dependent as no longer block on dependent.
  void retire(u8 dependent);

private:
  // Enforce the top-1 sorting invariant.
  void resort_queue();
  struct ScheduledEvent {
    // Technically a tick has 64 bits, but 2**56 is already insanely large (quadrillions)
    // Assuming a wildly fast tick rate of 100M/s, this would still take 32 days to overflow
    u64 tick : 48;
    u64 event_index : 8;
    u64 unused : 8; // Bits reserved for future use.
    auto operator<=>(const ScheduledEvent &o) const { return tick <=> o.tick; }
  };
  auto operator[](size_t idx) -> ScheduledEvent & { return _queue[idx]; }
  auto operator[](size_t idx) const -> const ScheduledEvent & { return _queue[idx]; }

  // Members to maintain directed acyclic graph of event dependencies.
  pepp::FixedBitset<MAX_EVENTS> _scheduled = {0};
  std::array<pepp::FixedBitset<MAX_EVENTS>, MAX_EVENTS> _dependencies;
  std::array<pepp::FixedBitset<MAX_EVENTS>, MAX_EVENTS> _dependents;

  u64 _current_tick = 0;
  // Helper class to pack tick+index.

  static_assert(sizeof(ScheduledEvent) == sizeof(u64));
  // Indices of currently scheduled events, partially sorted by tick. The next event to execute is stored in index 0
  // with the rest of the queue being unsorted.
  std::array<ScheduledEvent, MAX_EVENTS> _queue;
  // The current number of scheuled events. Alternatively, the highest index of _event_queue that is valid is
  // (_queue_size-1). If 0, no events are queued.
  u16 _queue_size = 0;
  mutable struct Counters {
    u64 scheduled = 0; // Count for schedule() and schedule_over() calls, including those which are paused/delayed.
    u64 executed = 0;  // Count for events executed, including those which are paused/delayed.
  } _counters;
};