#include "pep10isa.hpp"

Pep10CPU::Resumable Pep10CPU::Resumable::promise_type::get_return_object() {
  auto h = std::coroutine_handle<promise_type>::from_promise(*this);
  return Resumable{h};
}

bool Pep10CPU::Resumable::promise_type::NextEventAwaitable::await_ready() { return promise.current_event != nullptr; }

const Event *Pep10CPU::Resumable::promise_type::NextEventAwaitable::await_resume() {
  auto ret = promise.current_event;
  promise.current_event = nullptr; // Clear event after resuming, to prevent accidentally reusing it on the next await.
  return ret;
}

Pep10CPU::Resumable Pep10CPU::instruction_execute_coro(EventLoop &s) {
  while (true) {
    const Event *ev = co_await Resumable::promise_type::NextEvent{};
    auto await_is_read = read<u8>(s, pc, ev->event_id);
    u8 mn = co_await await_is_read;
    u8 requeue_cycle_delay = 0;
    pc++;
    if (mn < 0x80) {
      requeue_cycle_delay = 2, wcount += mn;
    } else {
      u16 operand = co_await read<u16>(s, pc, ev->event_id);
      pc += 2, requeue_cycle_delay = 4, wcount += operand << mn;
    }
    icount = icount + 1;
    clock.request_clock(id(), requeue_cycle_delay);
    // Force re-entry into event loop to ensure timing correctness.
    // TODO: find some clever way to combine with NextEvent!
    using Promise = Resumable::promise_type;
    auto typed = std::coroutine_handle<Promise>::from_address(_coro.handle.address());
    typed.promise().current_event = nullptr;
  }
}

void Pep10CPU::post(const Event *ev) {
  using Promise = Resumable::promise_type;
  auto typed = std::coroutine_handle<Promise>::from_address(_coro.handle.address());
  typed.promise().current_event = ev;
}

void Pep10CPU::handle_event(const Event *ev) {
  if (!_coro.handle) _coro = instruction_execute_coro(loop);

  post(ev), _coro.handle.resume();
}
