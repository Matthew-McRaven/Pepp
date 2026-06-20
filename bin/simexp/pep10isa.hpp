#pragma once

#include <coroutine>
#include <exception>
#include "./events.hpp"
#include "core/ds/hash/djb.hpp"
#include "event_dispatch.hpp"
#include "event_loop.hpp"
#include "sim_clocktree.hpp"
#include "sim_eventhandle.hpp"

struct DRAM : public EventHandlingDevice {
  DRAM(Descriptor descriptor) : EventHandlingDevice(descriptor) {}
  void handle_event(const Event *ev) override {
    if (ev->type == Event::Type::MemoryAccess) {
      auto mem_ev = reinterpret_cast<const MemoryRequest *>(ev);
      auto hash = pepp::djb(mem_ev->address);
      memcpy(mem_ev->buffer, (u8 *)&hash, std::min<u8>(mem_ev->len, sizeof(hash)));
    }
  }
};

template <typename Target> struct AccessSnooper : public EventDispatcher::Filter<AccessSnooper<Target>> {
  Device::ID _id{0};
  u64 access_count = 0;
  AccessSnooper(EventDispatcher &disp, Device::ID previous, Device::ID self_id)
      : EventDispatcher::Filter<AccessSnooper<Target>>(disp, previous), _id(self_id) {}
  Device::ID id() const override { return _id; }
  bool filter(const Event *ev) {
    if (ev->type == Event::Type::MemoryAccess) access_count++;
    return true;
  }
};

template <typename T> struct MemoryAwaiter {
  Event::ID dependent;
  Device::ID src_id;
  u32 addr;
  MemoryRequest::Kind type;
  u64 delay = 0;
  T result{};
  EventLoop &sim;
  // MemoryRequest *request = nullptr; // store event pointer
  MemoryAwaiter(EventLoop &s, Event::ID dependent, Device::ID src_id, u64 delay = 0)
      : dependent(dependent), src_id(src_id), delay(delay), sim(s) {}
  static MemoryAwaiter<T> read(EventLoop &s, Event::ID dependent, Device::ID src_id, u32 addr, u64 delay = 0) {
    MemoryAwaiter<T> ret(s, dependent, src_id, delay);
    ret.type = MemoryRequest::Kind::Read;
    ret.addr = addr;
    return ret;
  }

  bool await_ready() {
    return false;
    // "fake" a happy path where the result can be instantly computed -- like when the value is cached.
    if (type == MemoryRequest::Kind::Read) result = pepp::djb(addr);
    return false;
  }

  void await_suspend(std::coroutine_handle<> handle) {
    auto dependee = this->sim.allocator.alloc<MemoryRequest>();
    dependee->base.type = Event::Type::MemoryAccess;
    dependee->base.source = src_id;
    dependee->address = addr;
    dependee->len = sizeof(T);
    dependee->type = type;
    dependee->buffer = reinterpret_cast<u8 *>(&this->result);
    sim.scheduler.schedule_over(dependee->base.event_id, dependent, delay);
  }
  T await_resume() { return result; }
};

struct DelayAwaiter {
  Event::ID dependent;
  Device::ID src_id;
  u64 delay = 0;
  EventLoop &sim;
  DelayAwaiter(EventLoop &s, Event::ID dependent, Device::ID src_id, u64 delay = 0)
      : dependent(dependent), src_id(src_id), delay(delay), sim(s) {}
  bool await_ready() { return this->sim.scheduler.skip(this->delay); }
  void await_suspend(std::coroutine_handle<> handle) {
    auto dependee = this->sim.allocator.alloc<SequenceEvent>();
    dependee->base.type = Event::Type::Sequence;
    dependee->base.source = src_id;
    sim.scheduler.schedule_over(dependee->base.event_id, dependent, delay);
  }
  void await_resume() {}
};

struct Pep10CPU : public EventHandlingDevice {
  Pep10CPU(Descriptor descriptor, EventLoop &loop, pepp::ClockGovernor &clock)
      : EventHandlingDevice(descriptor), loop(loop), clock(clock) {}
  i16 regs[8];
  bool nzvc[4];
  u16 pc = 0;
  i64 icount = 0, wcount = 0;
  EventLoop &loop;
  pepp::ClockGovernor &clock;
  struct Resumable {
    // Just an alias to the coro handle already in _coro, but it makes this promise easier to use.
    std::coroutine_handle<> handle = nullptr;
    struct promise_type {
      EventLoop &sim;
      const Event *current_event = nullptr; // written before resume

      // First argument provided implicitly due to coro being a member fn
      promise_type(Pep10CPU &self, EventLoop &s) : sim(s) {}

      Resumable get_return_object();
      // Do not suspsend b/c we want to reach the first "input" point before returning to the outer loop.
      std::suspend_never initial_suspend() { return {}; }
      std::suspend_never final_suspend() noexcept { return {}; }
      void return_void() {}
      void unhandled_exception() { std::terminate(); }
      struct NextEvent {};
      struct NextEventAwaitable {
        promise_type &promise;
        bool await_ready();
        void await_suspend(std::coroutine_handle<> h) {}
        const Event *await_resume();
      };

      // Passthrough for any awaitable that isn't NextEvent
      template <typename T> auto await_transform(T &&t) { return std::forward<T>(t); }
      auto await_transform(NextEvent) { return NextEventAwaitable{*this}; }
    };
  } _coro{};

  template <typename T> MemoryAwaiter<T> read(EventLoop &s, u16 addr, Event::ID idx) {
    return MemoryAwaiter<T>::read(s, idx, id(), addr, 1);
  }
  DelayAwaiter delay(EventLoop &s, u64 ticks, Event::ID idx) { return DelayAwaiter(s, idx, id(), ticks); }
  Resumable instruction_execute_coro(EventLoop &s);
  void post(const Event *ev);
  void handle_event(const Event *ev) override;
};