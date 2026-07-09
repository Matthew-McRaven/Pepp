#include "./core_words.hpp"
#include "core/interactive_test/dict.hpp"
#include "core/interactive_test/interp.hpp"

/*
 * Manipulate elements on stack
 */
// ( a b -- b a)
void native_swap16(Interpreter *interp) {
  i16 first = interp->pop_psp<i16>();
  i16 second = interp->pop_psp<i16>();
  interp->push_psp(first);
  interp->push_psp(second);
}
inline static const NativeOpcode Swap16{
    .stack_delta = 0,
    .name = "swap",
    .h = native_swap16,
};

// (i16 -- i16 i16)
void native_dup16(Interpreter *interp) {
  i16 value = interp->pop_psp<i16>();
  interp->push_psp(value);
  interp->push_psp(value);
}

inline static const NativeOpcode Dup16{
    .stack_delta = 2,
    .name = "dup",
    .h = native_dup16,
};

// (i16 -- )
void native_drop16(Interpreter *interp) { i16 value = interp->pop_psp<i16>(); }

inline static const NativeOpcode Drop16{
    .stack_delta = -2,
    .name = "drop",
    .h = native_drop16,
};

// (a b -- a b a)
void native_over16(Interpreter *interp) {
  i16 first = interp->pop_psp<i16>();
  i16 second = interp->pop_psp<i16>();
  interp->push_psp(second);
  interp->push_psp(first);
  interp->push_psp(second);
}

inline static const NativeOpcode Over16{
    .stack_delta = 2,
    .name = "over",
    .h = native_over16,
};

void register_stack_words(Interpreter *p) {
  dict_insert_native(p, Swap16, {});
  dict_insert_native(p, Dup16, {});
  dict_insert_native(p, Drop16, {});
  dict_insert_native(p, Over16, {});
}
