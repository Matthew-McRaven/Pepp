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

Pep10CPU::Resumable Pep10CPU::instruction_execute_coro(DiscreteEventSimulator &s) {
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

void Pep10CPU::post(const Event *ev) {
  using Promise = Resumable::promise_type;
  auto typed = std::coroutine_handle<Promise>::from_address(_coro.handle.address());
  typed.promise().current_event = ev;
}

void Pep10CPU::handle_event(DiscreteEventSimulator &s, const Event *ev) {
  if (!_coro.handle) _coro = instruction_execute_coro(s);

  post(ev), _coro.handle.resume();
}
