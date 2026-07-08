#include <fmt/format.h>
#include <string>
#include "./core_words.hpp"
#include "core/interactive_test/dict.hpp"
#include "core/interactive_test/interp.hpp"

// (i16 -- )
void native_dot(Interpreter *interp) {
  auto tos = interp->pop_psp<i16>();
  interp->append_output(fmt::format("{}\n", tos));
}
inline static const NativeOpcode Dot{
    .stack_delta = -2,
    .name = ".",
    .h = native_dot,
};

// Usually this would buffer IO /inside/ the VM. However, to get INTERP working more quickly, we will delegate all IO to
// the host. ( -- i8 )
void native_key(Interpreter *interp) {
  // If our span of chararacters is empty, fgetch more input!
  if (interp->chars.empty()) {
    // Prefer to take input from the array of buffered strings
    if (!interp->buffered.empty()) {
      interp->storage = interp->buffered.front();
      interp->buffered.erase(interp->buffered.begin());
    }
    // Try to use interp's input source, if it exists
    else if (interp->input_source->has_input()) {
      interp->get_input();
    } else {
      // If no more chars, push 0 to indicate EOF
      interp->push_psp((char)0);
      interp->cb.alive = false;
      return;
    }
    // Ensure content is newline terminated
    if (!interp->storage.ends_with('\n')) interp->storage += '\n';
    interp->chars = interp->storage;
  }

  // Return those characters one at a time.
  if (!interp->chars.empty()) {
    char c = interp->chars.front();
    interp->chars.remove_prefix(1);
    interp->push_psp(c);
  } else {
    // If no more chars, push 0 to indicate EOF
    interp->push_psp((char)0);
  }
}
inline static const NativeOpcode Key{
    .stack_delta = 2,
    .name = "key",
    .h = native_key,
};
// (i8 -- )
void native_emit(Interpreter *interp) {
  const auto c = interp->pop_psp<char>();
  interp->append_output(std::string(1, c));
}
inline static const NativeOpcode Emit{
    .stack_delta = -2,
    .name = "emit",
    .h = native_emit,
};

// ( addr size -- i16)
void native_number(Interpreter *interp) {
  u16 size = interp->pop_psp<u16>();
  u16 addr = interp->pop_psp<u16>();
  auto res = number_helper(interp, addr, size);
  if (res.has_value()) interp->push_psp((i16)res.value());
  else interp->push_psp((i16)0);
}

inline static const NativeOpcode Number{
    .stack_delta = -2,
    .name = "number",
    .h = native_number,
};

// ( ptr -- )
// Print characters from memory at pointer until the first null byte.
void native_print_nullterminated(Interpreter *interp) {
  u16 addr = interp->pop_psp<u16>();
  while (true) {
    u8 c = interp->read<u8>(addr++);
    if (c == 0) break;
    interp->append_output(std::string(1, static_cast<char>(c)));
  }
}
inline static const NativeOpcode PrintNullTerminated{
    .stack_delta = -2,
    .name = "print0",
    .h = native_print_nullterminated,
};

void register_io_words(Interpreter *p) {
  dict_insert_native(p, Dot, {});
  dict_insert_native(p, Key, {});
  dict_insert_native(p, Emit, {});
  dict_insert_native(p, Number, {});
  dict_insert_native(p, PrintNullTerminated, {});
}
