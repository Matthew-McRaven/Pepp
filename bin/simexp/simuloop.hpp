#pragma once
#include <bit>
#include <cassert>
#include <concepts>
#include <coroutine>
#include <exception>
#include <functional>
#include <memory>
#include "core/ds/u64_bitset.hpp"
#include "core/integers.h"
#include "fmt/base.h"

using device_id_t = u8;
using path_t = u16;

class DiscreteEventSimulator;

// Any time you add a new event type, you must also modify the "slot" type in DES.√
struct Event {
  enum class Type : u8 {
    Invalid = 0,
    ReadRequest,
    WriteRequest,
    ClearRequest,
    Delay,
  } type;
  device_id_t source;
  u8 event_index;
};
template <typename T>
concept EventLike = requires(T t) {
  { t.base } -> std::same_as<Event &>;
} || std::derived_from<T, Event>;

u64 index_to_bitmask(u8 index) {
  if (index >= 64) [[unlikely]]
    throw std::out_of_range("Index must be less than 64");
  return 1ULL << index;
}

static_assert(std::is_standard_layout_v<Event>);
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

struct DelayEvent {
  Event base;
  u64 delay_ticks;
};

// Bitset of used slots. I want to use std::countr_zero to find the first free slot quickly.
// std::bitset does not include such a function, so we roll our own.
template <size_t COUNT> class IdiotBitset {
public:
  class Reference {
  public:
    Reference(IdiotBitset &bitset, size_t pos) : _bitset(bitset), _pos(pos) {}
    operator bool() const { return _bitset._storage & (1ULL << _pos); }
    bool operator~() const { return !(*this); }
    Reference &operator=(bool value) {
      _bitset.set(_pos, value);
      return *this;
    }
    Reference &operator=(const Reference &other) {
      bool value = static_cast<bool>(other);
      _bitset.set(_pos, value);
      return *this;
    }
    Reference &flip() {
      _bitset.flip(_pos);
      return *this;
    }

  private:
    IdiotBitset &_bitset;
    size_t _pos;
  };

  static_assert(COUNT <= 64, "COUNT must be <= 64 to fit in a u64 bitmap");
  u8 countr_zero() const { return std::countr_zero(_storage); }
  u8 countr_one() const { return std::countr_one(_storage); }
  u8 count() const { return std::popcount(_storage); }
  // Positional access
  void set(size_t pos, bool value = true) {
    _storage = value ? (_storage | (1ULL << pos)) : (_storage & ~(1ULL << pos));
  }
  void reset(size_t pos) { _storage &= ~(1ULL << pos); }
  void flip(size_t pos) { _storage ^= (1ULL << pos); }
  void test(size_t pos) { _storage &= (1ULL << pos); }
  // Access entire bitset
  bool all() const { return count() == COUNT; }
  bool any() const { return _storage != 0; }
  bool none() const { return _storage == 0; }
  void clear() { _storage = 0; }

  bool operator[](size_t pos) const { return (_storage >> pos) & 1; }
  Reference &operator[](size_t pos) { return Reference(this, pos); }

private:
  u64 _storage = 0; // Bitmap of used slots. 1 means used, 0 means free.
};

class DiscreteEventSimulator {
public:
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

  u64 current_tick() const noexcept { return _current_tick; }

  void post(u8 index) {}
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
    _event_slots_used.enable_bit(slot_index = _event_slots_used.countr_zero());
    EventSlot &slot = _event_slots[slot_index];
    // Avoiding UB by using placement new. construct_as technically has UB, but it's supposed to be "fine".
    // c++23 brings start_lifetime_as, which is the correct tool but not yet available on all platforms.
    auto ret = std::launder(new (slot.data) DerivedEvent{std::forward<Args>(args)...});
    ret->base.event_index = slot_index;
    return ret;
  }

  void free_event(Event *ev) {
    // Ensure that the event actually belongs to us.
    if (const void *address = ev; address == nullptr) return;
    else if (address < _event_slots[0].data || address >= _event_slots[MAX_EVENTS - 1].data + sizeof(EventSlot))
      throw std::runtime_error("Invalid event pointer");
    // Do not destroy until after we have computed its slot.
    size_t slot_index = (reinterpret_cast<std::byte *>(ev) - _event_slots[0].data) / sizeof(EventSlot);
    std::destroy_at(ev);
    _event_slots_used.reset(slot_index);
  }

  void reschedule(u8 dependent, u64 dependees, std::coroutine_handle<> resume) {}

private:
  u64 _current_tick = 0;

  static constexpr size_t MAX_EVENTS = 16;
  template <typename... Ts> struct Slot {
    // Helper to ensure that some type T is one of the passed types.
    template <typename T> static constexpr bool contains = (std::is_same_v<T, Ts> || ...);
    static constexpr std::size_t size = std::max({sizeof(Ts)...});
    static constexpr std::size_t alignment = std::max({alignof(Ts)...});
    // Round up size to next multiple of alignment — matches what compiler does
    static constexpr std::size_t padded_size = (size + alignment - 1) & ~(alignment - 1);
    alignas(alignment) std::byte data[padded_size];
  };
  using EventSlot = Slot<Event, MemoryRequest, DelayEvent>;

  pepp::FixedBitset<MAX_EVENTS> _event_slots_used;
  alignas(alignof(EventSlot)) std::array<EventSlot, MAX_EVENTS> _event_slots;
  // Ensure no padding is added to event slot,
  static_assert(sizeof(_event_slots) == MAX_EVENTS * sizeof(EventSlot));
};

struct DRAM {
  void handle_event(DiscreteEventSimulator &s, const Event *ev) {
    fmt::println("Handling command: device={}, type={}", ev->source, static_cast<u8>(ev->type));
  }
};

template <typename T> struct MemoryAwaiter {
  u8 dependent;
  T result{};
  u64 dependee_mask;
  DiscreteEventSimulator &sim;

  MemoryAwaiter(DiscreteEventSimulator &s, u8 dependent_id, u64 dependee_mask = 0)
      : dependent(dependent_id), dependee_mask(dependee_mask), sim(s) {}
  bool await_ready() { return false; } // always post

  void await_suspend(std::coroutine_handle<> handle) { sim.reschedule(dependent, dependee_mask, handle); }

  T await_resume() {
    // result was written into us before resume
    // sim frees the event after writing result
    return result;
  }
};

struct Bus {
  DiscreteEventSimulator *sim = nullptr;
  // Resumable register_callback(const Event &cmd) { co_return; }
  void handle_event(DiscreteEventSimulator &s, const Event *ev) {
    fmt::println("[{:02x}{:02x}] Selecting handler", ev->source, static_cast<u8>(ev->type));
    switch (ev->type) {
    case Event::Type::ReadRequest: {
      // Select next-level device(s) based on address(es)
      auto child = s.make_event<MemoryRequest>();
      child->base.source = 0x01;
      child->base.type = Event::Type::ReadRequest;
      // s->post_event(child);
    }
    case Event::Type::WriteRequest: {
      // Select next-level device(s) based on address(es)
      auto child = s.make_event<MemoryRequest>();
      child->base.source = 0x01;
      child->base.type = Event::Type::WriteRequest;
      // s->post_event(child);
    }
    case Event::Type::ClearRequest: break;
    case Event::Type::Invalid: break;
    case Event::Type::Delay: break;
    }

    // s->submit(register_callback(cmd));
  }
};

struct Pep10CPU {
  i16 regs[8];
  bool nzvc[4];
  u16 pc = 0;
  int id = 0;
  volatile i64 icount = 0;
  i64 wcount = 0;
  static constexpr std::size_t FRAME_SIZE = 512;

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
      std::suspend_always initial_suspend() { return {}; }
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
    auto dependee = s.make_event<MemoryRequest>();
    dependee->base.source = id;
    dependee->address = addr;
    auto ret = MemoryAwaiter<T>(s, idx, index_to_bitmask(idx));
    dependee->len = sizeof(T);
    dependee->type = MemoryRequest::Kind::Read;
    dependee->buffer = reinterpret_cast<u8 *>(&ret.result);
    s.post(dependee->base.event_index);
    return ret;
  }

  Resumable instruction_execute_coro(DiscreteEventSimulator &s) {
    while (true) {
      const Event *ev = co_await Resumable::promise_type::NextEvent{};
      fmt::println("{:04d}[{}] Intsr begin", s.current_tick(), id);
      auto await_is_read = read<u8>(s, pc, ev->event_index);
      u8 mn = co_await await_is_read;
      fmt::println("{:04d}[{}] op fetched", s.current_tick(), id);
      pc++;
      if (mn < 0x80) {
        fmt::println("{:04d}[{}] unary execute", s.current_tick(), id);
        //    co_await execute_unary(loop, mn);

        // co_await delay(loop, 2); // unary takes 2 cycles
        wcount += mn;
      } else {
        fmt::println("{:04d}[{}] opspec fetching", s.current_tick(), id);
        u16 operand = co_await read<u16>(s, pc, ev->event_index);
        pc += 2;
        fmt::println("{:04d}[{}] opspec fetched", s.current_tick(), id);
        fmt::println("{:04d}[{}] nonunary execute", s.current_tick(), id);
        // co_await delay(loop, 4); // nonunary takes 2 cycles
        wcount += operand << mn;
      }
      icount = icount + 1;
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