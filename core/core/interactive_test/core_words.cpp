#include "core_words.hpp"
#include "./interp.hpp"
#include "core/integers.h"

void native_add16i(Interpreter *interp) {
  i16 lhs = interp->pop_psp<i16>();
  i16 rhs = interp->pop_psp<i16>();
  i16 result = lhs + rhs;
  interp->push_psp(result);
}

void native_dup16(Interpreter *interp) {
  i16 value = interp->pop_psp<i16>();
  interp->push_psp(value);
  interp->push_psp(value);
}

void native_docol(Interpreter *interp) {
  // Push next ip onto return stack
  interp->push_rsp(interp->cb.nxt_ip);
  interp->cb.nxt_ip = interp->cb.w + 2;
}

void native_exitcol(Interpreter *interp) {
  // Pop top of RSP into next ip.
  interp->cb.nxt_ip = interp->pop_rsp<u16>();
}

void native_halt(Interpreter *interp) { interp->cb.alive = false; }

void native_swap16(Interpreter *interp) {
  i16 first = interp->pop_psp<i16>();
  i16 second = interp->pop_psp<i16>();
  interp->push_psp(first);
  interp->push_psp(second);
}

void native_drop16(Interpreter *interp) { i16 value = interp->pop_psp<i16>(); }

void native_over16(Interpreter *interp) {
  i16 first = interp->pop_psp<i16>();
  i16 second = interp->pop_psp<i16>();
  interp->push_psp(second);
  interp->push_psp(first);
  interp->push_psp(second);
}
