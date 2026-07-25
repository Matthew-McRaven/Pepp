#include "./core_words.hpp"
#include "core/interactive_test/dict.hpp"
#include "core/interactive_test/interp.hpp"

void native_here(Interpreter *interp) { interp->push_psp(interp->cb.here); }
inline static const NativeOpcode Here{
    .stack_delta = 2,
    .name = "here",
    .h = native_here,
};

// ( -- u16) push current psp value
void native_psp(Interpreter *interp) { interp->push_psp(interp->cb.psp); }
static const NativeOpcode PspVal{
    .stack_delta = 2,
    .name = "psp",
    .h = native_psp,
};
// ( -- u16) push current rsp value onto psp
void native_rsp(Interpreter *interp) { interp->push_psp(interp->cb.rsp); }
inline static const NativeOpcode RspVal{
    .stack_delta = 2,
    .name = "rsp",
    .h = native_rsp,
};

void native_lateststore(Interpreter *interp) {
  u16 value = interp->pop_psp<u16>();
  interp->cb.latest = value;
}

inline static const NativeOpcode LatestStore{
    .stack_delta = -2,
    .name = "latest!",
    .h = native_lateststore,
};

void register_sys_globals_words(Interpreter *p) {
  dict_insert_native(p, Here, {});
  dict_insert_native(p, PspVal, {});
  dict_insert_native(p, RspVal, {});
  dict_insert_native(p, LatestStore, {});
}
