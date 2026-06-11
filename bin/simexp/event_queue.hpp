
#pragma once
#include <array>
#include <functional>
#include "./event_dispatch.hpp"
#include "./events.hpp"
#include "core/ds/u64_bitset.hpp"
#include "core/integers.h"

class EventLoop {
  static constexpr size_t MAX_EVENTS = 32;

public:
  EventDispatcher dispatcher;

  EventLoop() = default;
  // Disable copy/move for now. I know I'll want to clone a simulator (which is a form of copy) at some point.
  EventLoop(const EventLoop &) = delete;
  EventLoop &operator=(const EventLoop &) = delete;
  EventLoop(EventLoop &&) = delete;
  EventLoop &operator=(EventLoop &&) = delete;

  struct Allocator {
    u8 count() const { return _slots_used.count(); }

    // Allocate a new event in the internal buffer but do not schedule that event.
    template <EventLike DerivedEvent, typename... Args> DerivedEvent *make_event(Args... args);
    void free_event(Event *ev);
    void free_event(u8 idx);

    Event *at(u8 idx);
    const Event *at(u8 idx) const;
    auto operator[](size_t idx) -> Event * { return reinterpret_cast<Event *>(_slots[idx].data); }
    auto operator[](size_t idx) const -> const Event * { return reinterpret_cast<const Event *>(_slots[idx].data); }

  private:
    pepp::FixedBitset<MAX_EVENTS> _slots_used;
    alignas(alignof(EventSlot)) std::array<EventSlot, MAX_EVENTS> _slots;
    // Ensure no padding is added to event slot,
    static_assert(sizeof(_slots) == MAX_EVENTS * sizeof(EventSlot));
    mutable struct Counters {
      u64 allocated = 0; // Call count for make_event
      u64 freed = 0;     // Count for events executed and then freed
    } _counters;
  } allocator;

  struct Scheduler {
    u8 count() const;
    u64 current_tick() const noexcept;
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

    // Pop the top element from the queue and update current tick. Return value is the index in allocator of event to be
    // handled.
    u8 consume_top();
    // Mark all dependees of dependent as no longer block on dependent.
    void resume(u8 dependent);

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
  } scheduler;

  enum class Status {};
  Status run(u64 max_ticks);
  Status run(std::function<bool()> pause);
  // While the std::function overload of run is more flexible, it introduces significant performance pessimization on
  // the hot path (~20% slower). This template allows the compiler to generate better code where the stopping condition
  // is a simple lambda.
  template <typename StopCondition> [[clang::noinline]] Status run(StopCondition &&stop);

  // A debug helper to print out the current state of the event loop.
  void dump_state() const;

private:
  u8 paused_events() const { return allocator.count() - scheduler.count(); }
};

template <EventLike DerivedEvent, typename... Args> DerivedEvent *EventLoop::Allocator::make_event(Args... args) {
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
  ret->base.event_index = slot_index;
  _counters.allocated++;
  return ret;
}

template <typename StopCondition> EventLoop::Status EventLoop::run(StopCondition &&stop) {
  while (!stop() && scheduler.count() > 0) {
    // dump_state();
    // 1. Determine which event should be processed next and advance _current_tick
    const auto ev_index = scheduler.consume_top();
    // 2. Execute or resume that event.
    const auto ev = allocator[ev_index];
    dispatcher.handle_event(ev);
    // 3. If event executed excuted to completion, release its dependents and possible free its slot.
    if (!scheduler.scheduled(ev_index)) {
      scheduler.resume(ev_index);
      if (!ev->recurs) allocator.free_event(ev_index);
    }
  }
  return {};
}