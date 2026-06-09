#pragma once

#include <coroutine>
#include <exception>
#include "./events.hpp"
#include "./simuloop.hpp"

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

  template <typename T> MemoryAwaiter<T> read(EventLoop &s, u16 addr, u8 idx) {
    return MemoryAwaiter<T>::read(s, idx, id, addr, 1);
  }
  DelayAwaiter delay(EventLoop &s, u64 ticks, u64 idx) { return DelayAwaiter(s, idx, id, ticks); }
  Resumable instruction_execute_coro(EventLoop &s);
  void post(const Event *ev);
  void handle_event(EventLoop &s, const Event *ev);
};