
#pragma once
#include <functional>
#include "./event_dispatch.hpp"
#include "core/integers.h"
#include "event_allocator.hpp"
#include "event_scheduler.hpp"

class EventLoop {

public:
  EventAllocator allocator;
  EventDispatcher dispatcher;
  EventScheduler scheduler;

  EventLoop() = default;
  // Disable copy/move for now. I know I'll want to clone a simulator (which is a form of copy) at some point.
  EventLoop(const EventLoop &) = delete;
  EventLoop &operator=(const EventLoop &) = delete;
  EventLoop(EventLoop &&) = delete;
  EventLoop &operator=(EventLoop &&) = delete;

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
  u8 paused_events() const { return allocator.current_allocated() - scheduler.current_scheduled(); }
};


template <typename StopCondition> EventLoop::Status EventLoop::run(StopCondition &&stop) {
  while (!stop() && scheduler.current_scheduled() > 0) {
    // dump_state();
    // 1. Determine which event should be processed next and advance _current_tick
    const auto ev_index = scheduler.pop_front();
    // 2. Execute or resume that event.
    const auto ev = allocator[ev_index];
    dispatcher.handle_event(ev);
    // 3. If event executed excuted to completion, release its dependents and possible free its slot.
    if (!scheduler.scheduled(ev_index)) {
      scheduler.retire(ev_index);
      if (!ev->recurs) allocator.free(ev_index);
    }
  }
  return {};
}