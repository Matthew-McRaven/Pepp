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
    .name = "0branch",
    .h = native_zbranch,
};

void native_execute(Interpreter *interp) {
  u16 xt = interp->pop_psp<u16>();
  interp->redirect_next_step = xt;
}
inline static const NativeOpcode Execute{
    .stack_delta = -2,
    .name = "execute",
    .h = native_execute,
};

void register_control_words(Interpreter *p) {
  dict_insert_native(p, ZBranch, {});
  dict_insert_native(p, Execute, {});
  // Compile 0-branch, saving location onto stack and compile a dummy offset
  p->run_on(": if immediate ' 0branch , here 0 ,		;");
  // Calculate offset from the address on the stack and back-fill the value
  p->run_on(": then immediate dup 2 + here swap - swap ! ;");
  p->run_on(": op immediate"
            " word number"    // Read next word into TOS
            " ' branch , 2 ," // Branch over the location where we will put the opcode
            " here swap ,"    // Write out the native opcode "under" the branch
            " , ;"            // Write out the pointer to the opcode we just wrote.
  );
}