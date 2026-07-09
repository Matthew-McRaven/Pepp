#include "./core_words.hpp"
#include "core/interactive_test/dict.hpp"
#include "core/interactive_test/interp.hpp"

// ( i16 i16 -- i16)
void native_add16i(Interpreter *interp) {
  i16 rhs = interp->pop_psp<i16>();
  i16 lhs = interp->pop_psp<i16>();
  i16 result = lhs + rhs;
  interp->push_psp(result);
}

inline static const NativeOpcode Add16i{
    .stack_delta = -2,
    .name = "+",
    .h = native_add16i,
};

// ( i16 i16 -- i16)
void native_sub16i(Interpreter *interp) {
  i16 rhs = interp->pop_psp<i16>();
  i16 lhs = interp->pop_psp<i16>();
  i16 result = lhs - rhs;
  interp->push_psp(result);
}
inline static const NativeOpcode Sub16i{
    .stack_delta = -2,
    .name = "-",
    .h = native_sub16i,
};

void register_arithmetic_words(Interpreter *p) {
  dict_insert_native(p, Add16i, {});
  dict_insert_native(p, Sub16i, {});
}
