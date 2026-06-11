#pragma once
#include <coroutine>
#include "./events.hpp"
#include "core/ds/hash/djb.hpp"
#include "core/integers.h"
#include "event_queue.hpp"

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
    sim.scheduler.schedule_over(dependee->base.event_index, dependent, delay);
  }
  T await_resume() { return result; }
};

struct DelayAwaiter {
  u8 dependent, src_id;
  u64 delay = 0;
  EventLoop &sim;
  DelayAwaiter(EventLoop &s, u8 dependent, u8 src_id, u64 delay = 0)
      : dependent(dependent), src_id(src_id), delay(delay), sim(s) {}
  bool await_ready() { return this->sim.scheduler.skip(this->delay); }
  void await_suspend(std::coroutine_handle<> handle) {
    auto dependee = this->sim.allocator.alloc<SequenceEvent>();
    dependee->base.type = Event::Type::SequenceEvent;
    dependee->base.source = src_id;
    sim.scheduler.schedule_over(dependee->base.event_index, dependent, delay);
  }
  void await_resume() {}
};
