#include "./core_words.hpp"
#include "core/interactive_test/dict.hpp"
#include "core/interactive_test/interp.hpp"

// (address data -- )
void native_store(Interpreter *interp) {
  u16 addr = interp->pop_psp<u16>();
  u16 value = interp->pop_psp<u16>();
  interp->write<u16>(value, addr);
}
inline static const NativeOpcode Store{
    .stack_delta = -4,
    .name = "!",
    .h = native_store,
};
// (src cnt dst -- )
// Should be compatible with `WORD <pushdest>`, assuming you can find the dest again.
void native_cmove(Interpreter *interp) {
  u16 dst = interp->pop_psp<u16>();
  u16 size = interp->pop_psp<u16>();
  u16 src = interp->pop_psp<u16>();
  auto src_span = std::span<const u8>(interp->memory.data() + src, size);
  interp->write(dst, src_span);
}
inline static const NativeOpcode CMove{
    .stack_delta = -6,
    .name = "cmove",
    .h = native_cmove,
};

// (src cnt dst -- )
// Same as cmove, but appends a 0 in dst.
void native_cmove0(Interpreter *interp) {
  u16 dst = interp->pop_psp<u16>();
  u16 size = interp->pop_psp<u16>();
  u16 src = interp->pop_psp<u16>();
  auto src_span = std::span<const u8>(interp->memory.data() + src, size);
  interp->write(dst, src_span);
  u16 null_terminator_addr = dst + size;
  interp->write<u8>(0, null_terminator_addr);
}
inline static const NativeOpcode CMove0{
    .stack_delta = -6,
    .name = "cmove0",
    .h = native_cmove,
};

u16 strlen_helper(Interpreter *interp, u16 addr) {
  u16 ret = 0;
  while (true) {
    u8 c = interp->read<u8>(addr++);
    if (c == 0) break;
    ret++;
  }
  return ret;
}
// ( ptr -- len)
void native_strlen(Interpreter *interp) {
  u16 addr = interp->pop_psp<u16>();
  interp->push_psp(strlen_helper(interp, addr));
}
inline static const NativeOpcode Strlen{
    .stack_delta = 0,
    .name = "strlen",
    .h = native_strlen,
};

void register_memory_words(Interpreter *p) {
  dict_insert_native(p, Store, {});
  dict_insert_native(p, CMove, {});
  dict_insert_native(p, CMove0, {});
  dict_insert_native(p, Strlen, {});
  p->run_on(": var word create drop" // Create a new dict entry from the next word. discard nt.
            " ' nest @ ,"            // Emit opcode of docol.
            " ' lit , "              // Emit lit
            " here 4 + ,"            // Make lit's value a pointer to the next cell
            " ' unnest ,"            // Emit unnest
            " here 0 ,"              // leave pointer to storage on stack, and allocate 2-bytes of storage.
            " ;");
}
