#include "./core_words.hpp"
#include "core/interactive_test/dict.hpp"
#include "core/interactive_test/interp.hpp"
#include "fmt/format.h"

void native_dumpdict(Interpreter *interp) {
  auto b = begin(interp), e = end(interp);
  while (b != e) {
    auto v = *b;
    interp->append_output(fmt::format("{:16}({:3}): 0x{:04x}", v.name(), (u16)v.strlen_flags(), b.link()));
    interp->append_output(fmt::format("  &pcode==0x{:04x}; pcode==0x{:04x}; *pcode=={}\n", (i16)v.pcode_addr(),
                                      (i16)v.pcode(), (i16)v.code0()));
    b++;
  }
}

inline static const NativeOpcode DumpDict{
    .stack_delta = 0,
    .name = "dumpdict",
    .h = native_dumpdict,
};

void native_toggle_debug(Interpreter *interp) { interp->cb.do_debug = !interp->cb.do_debug; }

inline static const NativeOpcode ToggleDebug{
    .stack_delta = 0,
    .name = "~debug",
    .h = native_toggle_debug,
};

void native_dumphex(Interpreter *interp) {
  auto len = interp->pop_psp<u16>();
  auto addr = interp->pop_psp<u16>();
  for (u16 i = 0; i < len; i++) {
    auto b = interp->read<u8>(addr + i);
    interp->append_output(fmt::format("{:02x} ", b));
  }
  interp->append_output("\n");
}
static const NativeOpcode DumpHex{
    .stack_delta = -4,
    .name = "dumphex",
    .h = native_dumphex,
};

void register_debug_words(Interpreter *p) {
  dict_insert_native(p, DumpDict, {});
  dict_insert_native(p, ToggleDebug, {});
  dict_insert_native(p, DumpHex, {});
}
