#include "core/interactive_test/dict.hpp"
#include "./core_words.hpp"
#include "core/interactive_test/interp.hpp"
#include "fmt/format.h"

// ( addr size -- nt|0)
void native_find(Interpreter *interp) {
  DictionaryIterator iter(interp);
  const auto end = DictionaryIterator(interp, 0);
  u16 size = interp->pop_psp<u16>();
  u16 addr = interp->pop_psp<u16>();
  u16 ret = find_helper(interp, addr, size);
  interp->push_psp(ret);
}

inline static const NativeOpcode Find{
    .stack_delta = -2,
    .name = "find",
    .h = native_find,
};
// (nt -- cfa)
void native_cfa(Interpreter *interp) {
  u16 nt_addr = interp->pop_psp<u16>();
  NiceDictHeader hdr(interp, nt_addr);
  interp->push_psp(hdr.pcode());
}

inline static const NativeOpcode CFA{
    .stack_delta = 0,
    .name = "cfa",
    .h = native_cfa,
};

void register_dict_words(Interpreter *interp) {
  dict_insert_native(interp, Find, {});
  dict_insert_native(interp, CFA, {});
}