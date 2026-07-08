#include "./core_words.hpp"
#include "core/interactive_test/dict.hpp"
#include "core/interactive_test/interp.hpp"
#include "fmt/format.h"

void native_dumpdict(Interpreter *interp) {
  auto b = begin(interp), e = end(interp);
  while (b != e) {
    auto v = *b;
    std::cout << fmt::format("{:9}({:3}): 0x{:04x}", v.name(), (u16)v.strlen_flags(), b.link())
              << fmt::format("  &pcode==0x{:04x}; pcode==0x{:04x}; *pcode=={}\n", (i16)v.pcode_addr(), (i16)v.pcode(),
                             (i16)v.code0());
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

void register_debug_words(Interpreter *p) {
  auto h_dumpdict = dict_insert_native(p, DumpDict, {});
  auto h_toggledebug = dict_insert_native(p, ToggleDebug, {});
}
