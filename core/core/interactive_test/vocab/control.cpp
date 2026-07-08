#include "./core_words.hpp"
#include "core/interactive_test/dict.hpp"
#include "core/interactive_test/interp.hpp"
#include "fmt/format.h"

void native_zbranch(Interpreter *interp) {
  // If TOS is 0, take the branch.
  if (i16 tos = interp->pop_psp<i16>(); tos == 0) {
    // Read offset from next slot after IP
    u16 *nxt_ip = &interp->cb.nxt_ip;
    u16 value = interp->read<u16>(*nxt_ip);
    // Increment by +2 to account for the read of the next slot.
    *nxt_ip = *nxt_ip + value + 2;
  }
  // Otherwise consume the offset without branching.
  else
    interp->cb.nxt_ip += 2;
}
inline static const NativeOpcode ZBranch{
    .stack_delta = 0,
    .name = "branch0",
    .h = native_zbranch,
};

void register_control_words(Interpreter *interp) { dict_insert_native(interp, ZBranch, {}); }