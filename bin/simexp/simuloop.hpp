#pragma once
#include <bit>
#include <concepts>
#include <coroutine>
#include <exception>
#include <functional>
#include <memory>
#include <variant>
#include "./events.hpp"
#include "core/ds/hash/djb.hpp"
#include "core/ds/u64_bitset.hpp"
#include "core/integers.h"
#include "fmt/base.h"

inline u64 index_to_bitmask(u8 index) {
  if (index >= 64) [[unlikely]]
    throw std::out_of_range("Index must be less than 64");
  return 1ULL << index;
}

class Pep10CPU;
class DiscreteEventSimulator {
  static constexpr size_t MAX_EVENTS = 32;

public:
  Pep10CPU *cpu = nullptr;
  void handle_event(const Event *ev);
  using EventMask = pepp::FixedBitset<MAX_EVENTS>;
  DiscreteEventSimulator() = default;
  // Disable copy/move for now. I know I'll want to clone a simulator (which is a form of copy) at some point.
  DiscreteEventSimulator(const DiscreteEventSimulator &) = delete;
  DiscreteEventSimulator &operator=(const DiscreteEventSimulator &) = delete;
  DiscreteEventSimulator(DiscreteEventSimulator &&) = delete;
  DiscreteEventSimulator &operator=(DiscreteEventSimulator &&) = delete;

  // Clock Management
  // TODO: also where all the clock tree stuff would go
  /*
   * Execution functions which enter the event loop
   */
  enum class Status {};
  Status run(u64 max_ticks);
  Status run(std::function<bool()> pause);
  // If no events are scheduled, just advance the clock.
  bool skip(u64 ticks);
  // While the function variant of run is more flexible, it introduces significant performance pessimization on the hot
  // path (~20% slower). This template allows the compiler to general better code where the stopping condition is a
  // simple lambda.
  template <typename StopCondition> [[clang::noinline]] Status run(StopCondition &&stop);

  u64 current_tick() const noexcept { return _current_tick; }

  // Take the index of an already-allocated event slot, and schedule it to run after the given delay in ticks.
  void schedule(u8 index, u64 delay = 0);
  u64 posted = 0, retired = 0, executed = 0;
  // Atomically "replace" an already schedule event with a new one.
  void schedule_over(u8 dependee, u8 dependent, std::coroutine_handle<> resume, u64 delay);
  bool scheduled(u8 index) const;
  // Take an scheduled-but-not-executing event index (dependent), and
  void mark_depends(u8 dependent, EventMask dependees);
  void pause(u8 dependent, EventMask dependees, std::coroutine_handle<> resume);

  template <typename DerivedEvent, typename... Args> DerivedEvent *make_event(Args... args) {
    // The type you are allocating should be an event
    static_assert(EventLike<DerivedEvent>);
    // That event should be one of the supported types in our event slot.
    static_assert(EventSlot::contains<DerivedEvent>);
    // Blindly copying the event data as bytes should just work, which requires standard layout.
    static_assert(std::is_standard_layout_v<DerivedEvent>);
    // We won't call your subclass's destructor, so notify users at compile time if they try to create such an event
    static_assert(std::is_trivially_destructible_v<DerivedEvent>);

    // Our event slots should be properly sized and aligned for your event type.
    static_assert(sizeof(DerivedEvent) <= sizeof(_event_slots[0]));
    static_assert(alignof(DerivedEvent) <= alignof(EventSlot));

    // Minimize the amount of work in the critical section. Allocate the slot index before entering that critical
    // section.
    size_t slot_index;
    if (_event_slots_used.count() == MAX_EVENTS) throw std::runtime_error("No free event slots available");
    _event_slots_used.enable_bit(slot_index = _event_slots_used.countr_one());

    EventSlot &slot = _event_slots[slot_index];
    // Avoiding UB by using placement new. construct_as technically has UB, but it's supposed to be "fine".
    // c++23 brings start_lifetime_as, which is the correct tool but not yet available on all platforms.
    auto ret = std::launder(new (slot.data) DerivedEvent{std::forward<Args>(args)...});
    ret->base.event_index = slot_index;
    _coro_slots[slot_index] = nullptr;
    // fmt::println("{:04x} Allocated event {}", current_tick(), slot_index);
    posted++;
    return ret;
  }
  void dump_state() const;

private:
  void lockless_dumpstate() const;
  // Create and free events in internal arena
  void free_event(Event *ev);
  void free_event(u8 idx);
  // Assumes you already hold the lock!
  void resort_queue();

  u64 _current_tick = 0;

  /*
   * Data members for allocating events within this class
   */

  template <typename... Ts> struct Slot {
    // Helper to ensure that some type T is one of the passed types.
    template <typename T> static constexpr bool contains = (std::is_same_v<T, Ts> || ...);
    static constexpr std::size_t size = std::max({sizeof(Ts)...});
    static constexpr std::size_t alignment = std::max({alignof(Ts)...});
    // Round up size to next multiple of alignment — matches what compiler does
    static constexpr std::size_t padded_size = (size + alignment - 1) & ~(alignment - 1);
    alignas(alignment) std::byte data[padded_size];
  };
  using EventSlot = Slot<Event, MemoryRequest, SequenceEvent, ClockEvent>;

  // mutable gem5::UncontendedMutex _event_slots_mutex;
  EventMask _event_slots_used;
  alignas(alignof(EventSlot)) std::array<EventSlot, MAX_EVENTS> _event_slots;
  std::array<std::coroutine_handle<>, MAX_EVENTS> _coro_slots{nullptr};
  // Ensure no padding is added to event slot,
  static_assert(sizeof(_event_slots) == MAX_EVENTS * sizeof(EventSlot));
  /*
   * Members for maintaining event dependencies.
   * When an event
   */
  std::array<EventMask, MAX_EVENTS> _event_dependencies;
  std::array<EventMask, MAX_EVENTS> _event_dependents;

  /*
   * Data members for maintaining the actual event queue.
   */
  struct ScheduledEvent {
    u64 tick : 48;
    u64 event_index : 8;
    auto operator<=>(const ScheduledEvent &o) const { return tick <=> o.tick; }
  };
  // This event queue is effectively double-ended. The left/low side contains events which are scheduled and ready to be
  // consumed.
  // The paused section is entirely unsorted, while the scheduled section is partially sorted (top 1) by lowest tick.
  std::array<ScheduledEvent, MAX_EVENTS> _event_queue;
  u16 _queue_size = 0;
};
template <typename StopCondition> DiscreteEventSimulator::Status DiscreteEventSimulator::run(StopCondition &&stop) {
  while (!stop() && _queue_size > 0) {
    // dump_state();
    //  fmt::println("{:04x} Starting evloop", current_tick());
    /*
     * 1. Determine which event should be processed next and advance _current_tick.
     */
    const auto scheduled_idx = _event_queue[0].event_index;
    _current_tick = _event_queue[0].tick, executed++;
    // If there are item left in the queue, maintain top-1 sorting requirement
    if (_queue_size-- > 1) {
      // Move the last scheduled event to fill the hole caused by popping front
      _event_queue[0] = _event_queue[_queue_size];
      resort_queue();
      // std::partial_sort(_event_queue.begin(), _event_queue.begin() + 1, _event_queue.begin() + _queue_size);
    }

    // fmt::println("{:04x} Selected event index {} for execution", ev_time, scheduled.event_index);

    /*
     * 2. Execute or resume that event.
     */
    // This event was paused due to dependencies. Resume its coroutine.
    if (auto coro = _coro_slots[scheduled_idx]; coro) {
      _coro_slots[scheduled_idx] = nullptr;
      coro.resume();
    } else {
      const Event *ev = reinterpret_cast<const Event *>(_event_slots[scheduled_idx].data);
      // TODO: find the associated handler for that event and call handle_event
      handle_event(ev);
    }

    /*
     * 3. Check if the event executed to completion. If so, unmark dependencies and free the slot.
     */
    int promoted = 0;
    // std::lock_guard lock(_event_slots_mutex);
    //  This event executed to completion. Unmark any events which depend on it,
    if (!_coro_slots[scheduled_idx]) {

      EventMask waiters = _event_dependents[scheduled_idx];
      _event_dependents[scheduled_idx].clear();

      for (u64 bits = waiters(); bits;) {
        u8 paused_idx = std::countr_zero(bits);
        bits &= bits - 1;
        _event_dependencies[paused_idx].reset(scheduled_idx);
        if (_event_dependencies[paused_idx].none()) {
          promoted++;
          const auto dest_idx = _queue_size++;
          new (&_event_queue[dest_idx]) ScheduledEvent(_current_tick, paused_idx);
        }
        free_event(scheduled_idx);
      }

      if (promoted > 0) resort_queue();
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
  DiscreteEventSimulator &sim;
  // MemoryRequest *request = nullptr; // store event pointer
  MemoryAwaiter(DiscreteEventSimulator &s, u8 dependent, u8 src_id, u64 delay = 0)
      : dependent(dependent), src_id(src_id), delay(delay), sim(s) {}
  static MemoryAwaiter<T> read(DiscreteEventSimulator &s, u8 dependent, u8 src_id, u32 addr, u64 delay = 0) {
    MemoryAwaiter<T> ret(s, dependent, src_id, delay);
    ret.type = MemoryRequest::Kind::Read;
    ret.addr = addr;
    return ret;
  }

  bool await_ready() {
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
    sim.schedule_over(dependee->base.event_index, dependent, handle, delay);
  }
  T await_resume() { return result; }
};

struct DelayAwaiter {
  u8 dependent, src_id;
  u64 delay = 0;
  DiscreteEventSimulator &sim;
  DelayAwaiter(DiscreteEventSimulator &s, u8 dependent, u8 src_id, u64 delay = 0)
      : dependent(dependent), src_id(src_id), delay(delay), sim(s) {}
  bool await_ready() { return this->sim.skip(this->delay); }
  void await_suspend(std::coroutine_handle<> handle) {
    auto dependee = this->sim.make_event<SequenceEvent>();
    dependee->base.type = Event::Type::SequenceEvent;
    dependee->base.source = src_id;
    sim.schedule_over(dependee->base.event_index, dependent, handle, delay);
  }
  void await_resume() {}
};

struct Pep10CPU {
  i16 regs[8];
  bool nzvc[4];
  u16 pc = 0;
  int id = 0;
  i64 icount = 0, wcount = 0;

  struct Resumable {
    // Just an alias to the coro handle already in _coro, but it makes this promise easier to use.
    std::coroutine_handle<> handle = nullptr;
    struct promise_type {
      DiscreteEventSimulator &sim;
      const Event *current_event = nullptr; // written before resume

      // First argument provided implicitly due to coro being a member fn
      promise_type(Pep10CPU &self, DiscreteEventSimulator &s) : sim(s) {}

      Resumable get_return_object() {
        auto h = std::coroutine_handle<promise_type>::from_promise(*this);
        return Resumable{h};
      }
      // Do not suspsend b/c we want to reach the first "input" point before returning to the outer loop.
      std::suspend_never initial_suspend() { return {}; }
      std::suspend_never final_suspend() noexcept { return {}; }
      void return_void() {}
      void unhandled_exception() { std::terminate(); }
      struct NextEvent {};
      struct NextEventAwaitable {
        promise_type &promise;
        bool await_ready() { return promise.current_event != nullptr; }
        void await_suspend(std::coroutine_handle<> h) {}
        const Event *await_resume() {
          auto ret = promise.current_event;
          promise.current_event =
              nullptr; // Clear event after resuming, to prevent accidentally reusing it on the next await.
          return ret;
        }
      };

      // Passthrough for any awaitable that isn't NextEvent
      template <typename T> auto await_transform(T &&t) { return std::forward<T>(t); }
      auto await_transform(NextEvent) { return NextEventAwaitable{*this}; }
    };
  } _coro;

  template <typename T> MemoryAwaiter<T> read(DiscreteEventSimulator &s, u16 addr, u8 idx) {
    return MemoryAwaiter<T>::read(s, idx, id, addr, 1);
  }
  DelayAwaiter delay(DiscreteEventSimulator &s, u64 ticks, u64 idx) { return DelayAwaiter(s, idx, id, ticks); }
  Resumable instruction_execute_coro(DiscreteEventSimulator &s) {
    while (true) {
      const Event *ev = co_await Resumable::promise_type::NextEvent{};
      // fmt::println("{:04x}[{}] Intsr begin", s.current_tick(), id);
      auto await_is_read = read<u8>(s, pc, ev->event_index);
      u8 mn = co_await await_is_read;
      // fmt::println("{:04x}[{}] op fetched", s.current_tick(), id);
      pc++;
      if (mn < 0x80) {
        // fmt::println("{:04x}[{}] unary execute", s.current_tick(), id);
        //     co_await execute_unary(loop, mn);
        co_await delay(s, 2, ev->event_index); // unary takes 2 cycles
        wcount += mn;
      } else {
        // fmt::println("{:04x}[{}] opspec fetching", s.current_tick(), id);
        u16 operand = co_await read<u16>(s, pc, ev->event_index);
        pc += 2;
        // fmt::println("{:04x}[{}] opspec fetched", s.current_tick(), id);
        // fmt::println("{:04x}[{}] nonunary execute", s.current_tick(), id);
        co_await delay(s, 4, ev->event_index); // nonunary takes 4 cycles
        wcount += operand << mn;
      }
      icount = icount + 1;
      s.schedule(ev->event_index, 0);
    }
  }
  void post(const Event *ev) {
    using Promise = Resumable::promise_type;
    auto typed = std::coroutine_handle<Promise>::from_address(_coro.handle.address());
    typed.promise().current_event = ev;
  }
  void handle_event(DiscreteEventSimulator &s, const Event *ev) {
    if (!_coro.handle) _coro = instruction_execute_coro(s);

    post(ev), _coro.handle.resume();
  }
};