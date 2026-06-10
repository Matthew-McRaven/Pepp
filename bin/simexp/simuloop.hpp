#pragma once
#include <array>
#include <bit>
#include <coroutine>
#include <functional>
#include <utility>
#include "./events.hpp"
#include "core/ds/hash/djb.hpp"
#include "core/ds/u64_bitset.hpp"
#include "core/integers.h"

inline u64 index_to_bitmask(u8 index) {
  if (index >= 64) [[unlikely]]
    throw std::out_of_range("Index must be less than 64");
  return 1ULL << index;
}

class Pep10CPU;
class EventLoop {
  static constexpr size_t MAX_EVENTS = 32;

public:
  struct EventEntry {
    u8 source;
    Event::Type type;
    bool operator==(EventEntry const &other) const = default;
  };
  // The handler function for a specific device ID.
  std::vector<EventHandler *> devices;
  // A vector sized to event_type_count() * # of devices.
  // The handle for (device, ev) is at [device*event_type_count() + (u8)ev).
  // Since 0 is a reserved device ID, we use 0 as a sentinel value to indicate that no handler is registered for a
  // device/event pair. When handlers gets too large and starts having poor memory performance, switch to
  // ankerl::unordered_dense::map<EventEntry, u8, EventEntry::Hash>.
  std::vector<u8> handlers;

  void register_device(u8 source, EventHandler *handler);
  void register_handler(u8 source, Event::Type ev, u8 handler);
  void handle_event(const Event *ev);
  using EventMask = pepp::FixedBitset<MAX_EVENTS>;
  EventLoop() = default;
  // Disable copy/move for now. I know I'll want to clone a simulator (which is a form of copy) at some point.
  EventLoop(const EventLoop &) = delete;
  EventLoop &operator=(const EventLoop &) = delete;
  EventLoop(EventLoop &&) = delete;
  EventLoop &operator=(EventLoop &&) = delete;

  // Allocate a new event in the internal buffer but do not schedule that event.
  template <EventLike DerivedEvent, typename... Args> DerivedEvent *make_event(Args... args);

  // Free'ing decisions can only be made by the event loop when no dependencies point to this event, which happens at
  // the end of an iteration of run.
private:
  // Free the used event slot. Dependencies should already have been cleared by the caller.
  void free_event(Event *ev);
  void free_event(u8 idx);

public:
  u64 current_tick() const noexcept;
  // Take the index of an allocated event and schedule it to run after a given tick delay.
  void schedule(u8 index, u64 delay = 0);
  // Returns true if the event index is currently scheduled for execution.
  bool scheduled(u8 index) const;
  // Remove dependent event from the schedule until all events in dependees have executed, at which point dependent is
  // re-scheduled for execution. If resume is not a nullptr, that coroutine will be executed rather the the original
  // event handler.
  void pause(u8 dependent, EventMask dependees);
  // Mark dependent as paused on dependee, and schedule dependee for execution with a delay.
  // More efficient than a pause() followed by a schedule()
  void schedule_over(u8 dependee, u8 dependent, u64 delay);

  struct Counters {
    u64 allocated = 0; // Call count for make_event
    u64 executed = 0;  // Count for events executed, including those which are paused/delayed.
    u64 freed = 0;     // Count for events executed and then freed
  };
  mutable Counters _counters;

  // If no events are scheduled, advance the clock and return true else return false.
  // Can be used to efficiently advance time when there are no pending events.
  bool skip(u64 ticks);
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
  u8 paused_events() const { return _event_slots_used.count() - _queue_size; }
  EventMask _event_slots_used;
  alignas(alignof(EventSlot)) std::array<EventSlot, MAX_EVENTS> _event_slots;
  EventMask _scheduled_events = {0};
  // Ensure no padding is added to event slot,
  static_assert(sizeof(_event_slots) == MAX_EVENTS * sizeof(EventSlot));
  /*
   * Members for maintaining event dependencies.
   * When an event
   */
  std::array<EventMask, MAX_EVENTS> _event_dependencies;
  std::array<EventMask, MAX_EVENTS> _event_dependents;

  // Helper class to pack tick+index.
  struct ScheduledEvent {
    // Technically a tick has 64 bits, but 2**56 is already insanely large (quadrillions)
    // Assuming a wildly fast tick rate of 100M/s, this would still take 32 days to overflow
    u64 tick : 48;
    u64 event_index : 8;
    u64 unused : 8; // Bits reserved for future use.
    auto operator<=>(const ScheduledEvent &o) const { return tick <=> o.tick; }
  };
  static_assert(sizeof(ScheduledEvent) == sizeof(u64));

  u64 _current_tick = 0;
  // The current number of scheuled events. Alternatively, the highest index of _event_queue that is valid is
  // (_queue_size-1). If 0, no events are queued.
  u16 _queue_size = 0;
  // Indices of currently scheduled events, partially sorted by tick. The next event to execute is stored in index 0
  // with the rest of the queue being unsorted.
  std::array<ScheduledEvent, MAX_EVENTS> _event_queue;
  // Enforce the top-1 sorting invariant.
  void resort_queue();
};

template <EventLike DerivedEvent, typename... Args> DerivedEvent *EventLoop::make_event(Args... args) {
  static_assert(!std::is_same_v<DerivedEvent, Event>, "Can't allocate a base event");
  // That event should be one of the supported types in our event slot.
  static_assert(EventSlot::contains<DerivedEvent>);
  // Our event slots should be properly sized and aligned for your event type.
  static_assert(sizeof(DerivedEvent) <= sizeof(_event_slots[0]));
  static_assert(alignof(DerivedEvent) <= alignof(EventSlot));

  if (_event_slots_used.count() == MAX_EVENTS) throw std::runtime_error("No free event slots available");
  size_t slot_index = _event_slots_used.countr_one();
  _event_slots_used.enable_bit(slot_index);

  EventSlot &slot = _event_slots[slot_index];
  // Avoiding UB by using placement new. construct_as technically has UB, but it's supposed to be "fine".
  // c++23 brings start_lifetime_as, which is the correct tool but not yet available on all platforms.
  auto ret = std::launder(new (slot.data) DerivedEvent{std::forward<Args>(args)...});
  ret->base.event_index = slot_index;
  _counters.allocated++;
  return ret;
}

template <typename StopCondition> EventLoop::Status EventLoop::run(StopCondition &&stop) {
  while (!stop() && _queue_size > 0) {
    // dump_state();
    /*
     * 1. Determine which event should be processed next and advance _current_tick.
     */
    resort_queue();
    const auto scheduled_idx = _event_queue[0].event_index;
    _current_tick = _event_queue[0].tick, _counters.executed++;
    // If there are item left in the queue, maintain top-1 sorting requirement
    if (_queue_size-- > 1) _event_queue[0] = _event_queue[_queue_size];
    const Event *ev = reinterpret_cast<const Event *>(_event_slots[scheduled_idx].data);
    /*
     * 2. Execute or resume that event.
     */
    _scheduled_events.clear_bit(scheduled_idx);
    handle_event(ev);

    /*
     * 3. Check if the event executed to completion. If so, unmark dependencies and free the slot.
     */
    //  This event executed to completion. Unmark any events which depend on it,
    if (!_scheduled_events[scheduled_idx]) {
      for (u64 bits = _event_dependents[scheduled_idx](); bits;) {
        u8 paused_idx = std::countr_zero(bits);
        bits &= bits - 1;
        _event_dependencies[paused_idx].reset(scheduled_idx);
        if (_event_dependencies[paused_idx].none()) {
          new (&_event_queue[_queue_size++]) ScheduledEvent(_current_tick, paused_idx);
        }
      }
      if (!ev->recurs) free_event(scheduled_idx);
    }
  }
  return {};
}

template <typename T> struct MemoryAwaiter {
  u8 dependent, src_id;
  u32 addr;
  MemoryRequest::Kind type;
  u64 delay = 0;
  T result{};
  EventLoop &sim;
  // MemoryRequest *request = nullptr; // store event pointer
  MemoryAwaiter(EventLoop &s, u8 dependent, u8 src_id, u64 delay = 0)
      : dependent(dependent), src_id(src_id), delay(delay), sim(s) {}
  static MemoryAwaiter<T> read(EventLoop &s, u8 dependent, u8 src_id, u32 addr, u64 delay = 0) {
    MemoryAwaiter<T> ret(s, dependent, src_id, delay);
    ret.type = MemoryRequest::Kind::Read;
    ret.addr = addr;
    return ret;
  }

  bool await_ready() {
    return false;
    // "fake" a happy path where the result can be instantly computed -- like when the value is cached.
    if (type == MemoryRequest::Kind::Read) result = pepp::djb(addr);
    return this->sim.skip(this->delay);
  }

  void await_suspend(std::coroutine_handle<> handle) {
    auto dependee = this->sim.make_event<MemoryRequest>();
    dependee->base.type = Event::Type::MemoryAccess;
    dependee->base.source = src_id;
    dependee->address = addr;
    dependee->len = sizeof(T);
    dependee->type = type;
    dependee->buffer = reinterpret_cast<u8 *>(&this->result);
    sim.schedule_over(dependee->base.event_index, dependent, delay);
  }
  T await_resume() { return result; }
};

struct DelayAwaiter {
  u8 dependent, src_id;
  u64 delay = 0;
  EventLoop &sim;
  DelayAwaiter(EventLoop &s, u8 dependent, u8 src_id, u64 delay = 0)
      : dependent(dependent), src_id(src_id), delay(delay), sim(s) {}
  bool await_ready() { return this->sim.skip(this->delay); }
  void await_suspend(std::coroutine_handle<> handle) {
    auto dependee = this->sim.make_event<SequenceEvent>();
    dependee->base.type = Event::Type::SequenceEvent;
    dependee->base.source = src_id;
    sim.schedule_over(dependee->base.event_index, dependent, delay);
  }
  void await_resume() {}
};
