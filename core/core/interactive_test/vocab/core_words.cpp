#include "core_words.hpp"
#include <charconv>
#include "../interp.hpp"
#include "core/integers.h"
#include "core/interactive_test/dict.hpp"
#include "fmt/format.h"

void native_halt(Interpreter *interp) { interp->cb.alive = false; }
inline static const NativeOpcode Halt{
    .stack_delta = 0,
    .name = "halt",
    .h = native_halt,
};

// ( -- )
void native_nest(Interpreter *interp) {
  // Push next ip onto return stack
  interp->push_rsp(interp->cb.nxt_ip);
  interp->cb.nxt_ip = interp->cb.w + 2;
}
inline static const NativeOpcode Nest{
    .stack_delta = 0,
    .name = "nest",
    .h = native_nest,
};

// ( -- )
void native_unnest(Interpreter *interp) {
  // Pop top of RSP into next ip.
  interp->cb.nxt_ip = interp->pop_rsp<u16>();
}
inline static const NativeOpcode Unnest{
    .stack_delta = 0,
    .name = "unnest",
    .h = native_unnest,
};

// ( -- addr size )
void native_word(Interpreter *interp, u16 buffer_addr) {
  auto size = word_helper(interp, buffer_addr);
  interp->push_psp(buffer_addr);
  interp->push_psp(size);
}

// ( addr size -- nt)
void native_create(Interpreter *interp) {
  u16 size = interp->pop_psp<u16>();
  u16 addr = interp->pop_psp<u16>();
  std::string_view name(reinterpret_cast<const char *>(interp->memory.data() + addr), size);
  Flags flags = (Flags)0;
  auto ret = dict_header(interp, name, flags);
  // By default, code is place /after/ the header.
  interp->write_here_pp<u16>(interp->cb.here + 2);
  interp->push_psp(ret.nt());
  interp->cb.latest = ret.nt();
}
inline static const NativeOpcode Create{
    .stack_delta = -2,
    .name = "create",
    .h = native_create,
};

void native_hidden(Interpreter *interp) {
  u16 nt_addr = interp->pop_psp<u16>();
  auto hdr = NiceDictHeader(interp, nt_addr);
  hdr.toggle_hidden();
}
inline static const NativeOpcode Hidden{
    .stack_delta = -2,
    .name = "hidden",
    .h = native_hidden,
};

// ( -- nt) Push ptr to latest dictionary entry onto param stack.
void native_latest(Interpreter *interp) { interp->push_psp(interp->cb.latest); }
inline static const NativeOpcode Latest{
    .stack_delta = 2,
    .name = "latest",
    .h = native_latest,
};

// Read the byte after this opcode and push it onto the param stack.
// ( -- i16)
void native_lit(Interpreter *interp) {
  u16 *nxt_ip = &interp->cb.nxt_ip;
  u16 value = interp->read<u16>(*nxt_ip);
  *nxt_ip += 2;
  interp->push_psp(value);
}
inline static const NativeOpcode Lit{
    .stack_delta = 2,
    .name = "lit",
    .h = native_lit,
};

void native_fetch(Interpreter *interp) {
  i16 addr = interp->pop_psp<i16>();
  i16 value = interp->read<i16>(addr);
  interp->push_psp(value);
}
inline static const NativeOpcode Fetch{
    .stack_delta = 0,
    .name = "@",
    .h = native_fetch,
};

// (i16 -- ), pop data and write to here++
void native_comma(Interpreter *interp) {
  u16 value = interp->pop_psp<u16>();
  interp->write_here_pp<u16>(value);
}
inline static const NativeOpcode Comma{
    .stack_delta = -2,
    .name = ",",
    .h = native_comma,
};

void native_lbrac(Interpreter *interp) { interp->cb.state = (u8)Interpreter::State::Immediate; }
inline static const NativeOpcode Lbrac{
    .stack_delta = 0,
    .name = "[",
    .h = native_lbrac,
};
void native_rbrac(Interpreter *interp) { interp->cb.state = (u8)Interpreter::State::Compiling; }
inline static const NativeOpcode Rbrac{
    .stack_delta = 0,
    .name = "]",
    .h = native_rbrac,
};

void native_interpret(Interpreter *interp, u16 word_buffer, u16 pcode_lit) {
  auto size = word_helper(interp, word_buffer);
  auto nt_addr = find_helper(interp, word_buffer, size);
  std::string_view word_buffer_str(reinterpret_cast<const char *>(interp->memory.data() + word_buffer), size);

  // Input string was empty/null. Do not parse.
  if (word_buffer_str.empty() || (word_buffer_str.size() == 1 && word_buffer_str[0] == '\0')) return;
  else if (nt_addr == 0) { // Not found, try to parse as a number.
    auto num = number_helper(interp, word_buffer, size);
    // If compiling, must comple as a literal.
    if (!num.has_value()) {
      // Force simulator to go back to entry point.
      interp->cb.nxt_ip = 0;
      interp->append_output(fmt::format("{} ?\n", word_buffer_str));
      interp->chars = std::string_view(); // Clear chars to force new input on next iteration.
      return;
    } else if (interp->cb.state == (u8)Interpreter::State::Compiling) {
      interp->write_here_pp(pcode_lit);
      interp->write_here_pp<u16>(num.value_or(0));
    } else {
      interp->push_psp<u16>(num.value_or(0));
    }
  } else {
    // Get dict entry for the found word.
    auto hdr = NiceDictHeader(interp, nt_addr);
    // If it is immediate, or we are in immediate mode, execute the word.
    // Replicate machinery of step b/c we need to control dispatch.
    if (hdr.immediate() || interp->cb.state == (u8)Interpreter::State::Immediate) {
      auto cfa = hdr.pcode();
      interp->cb.w = cfa;
      auto opcode = interp->read<u16>(cfa);
      interp->dispatch(opcode);
    } else { // Otherwise, compile it into the current definition.
      interp->write_here_pp<u16>(hdr.pcode());
    }
  }
}

void native_rspinitval(Interpreter *interp) { interp->push_psp(Interpreter::INITIAL_RSP); }
inline static const NativeOpcode RspInitVal{
    .stack_delta = 2,
    .name = "r0",
    .h = native_rspinitval,
};

void native_rspstoreval(Interpreter *interp) { interp->cb.rsp = interp->pop_psp<u16>(); }
inline static const NativeOpcode RspStoreVal{
    .stack_delta = -2,
    .name = "rsp!",
    .h = native_rspstoreval,
};

void native_branch(Interpreter *interp) {
  // Read offset from next slot after IP
  u16 *nxt_ip = &interp->cb.nxt_ip;
  u16 value = interp->read<u16>(*nxt_ip);
  // Increment by +2 to account for the read of the next slot.
  *nxt_ip = *nxt_ip + value + 2;
}
inline static const NativeOpcode Branch{
    .stack_delta = 0,
    .name = "branch",
    .h = native_branch,
};

void push_constant(Interpreter *interp, u16 addr) { interp->push_psp(addr); }

u16 word_helper(Interpreter *interp, u16 buffer_addr) {
  u16 initial = buffer_addr;
  // Consume leading whitespace
  while (true) {
    native_key(interp);
    char c = interp->pop_psp<char>();
    if (c == 0 || !std::isspace(c)) {
      // Push back the first non-whitespace character to the buffer
      interp->write<char>(c, buffer_addr++);
      break;
    }
  }
  // Now consume characters until whitespace or null is encountered, writing them to the buffer

  while (true) {
    native_key(interp);
    char c = interp->pop_psp<char>();
    if (c == 0 || std::isspace(c)) break;
    else {
      // Write character to buffer
      interp->write<char>(c, buffer_addr++);
    }
  }
  // Ensure word is null terminated
  interp->write<char>(0, buffer_addr++);
  u16 size = buffer_addr - initial - 1; // Exclude null terminator
  return size;
}

std::optional<i32> number_helper(Interpreter *interp, u16 addr, u16 size) {
  std::string_view str(reinterpret_cast<const char *>(interp->memory.data() + addr), size);
  i32 base = 10, prefix_size = 0, sign = 1;
  // Extract base from prefix, and handle explict signs for decimals.
  if (str.starts_with("0b") || str.starts_with("0B")) base = 2, prefix_size = 2;
  else if (str.starts_with("0o") || str.starts_with("0O")) base = 8, prefix_size = 2;
  else if (str.starts_with("0x") || str.starts_with("0X")) base = 16, prefix_size = 2;
  else if (str.starts_with("-")) prefix_size = 1, sign = -1;
  else if (str.starts_with("+")) prefix_size = 1;

  // Convert the string view into an integer of the correct base
  auto sub = str.substr(prefix_size);
  i32 ret;
  const auto *sub_end = sub.data() + sub.size();
  auto [ptr, ec] = std::from_chars(sub.data(), sub_end, ret, base);
  // Catch error even if we don't do anything with it.
  if (ec != std::errc() || ptr != sub_end) return std::nullopt;
  return sign * ret;
}

u16 find_helper(Interpreter *interp, u16 addr, u16 size) {
  DictionaryIterator iter(interp);
  const auto end = DictionaryIterator(interp, 0);
  std::string_view str(reinterpret_cast<const char *>(interp->memory.data() + addr), size);
  for (; iter != end; ++iter) {
    auto hdr = *iter;
    // Must compare lenghts before names, else HIDDEN bit will not work
    if ((hdr.strlen_flags() & (u8)Flags::LEN) != size) continue;
    std::string_view name = hdr.name();
    if (name == str) return hdr.link_addr();
  }
  // Not found, push 0.
  return 0;
}

void register_core_words(Interpreter *p) {
  // Words with automatic registration.
  dict_insert_native(p, Halt, {});
  auto h_docol = dict_insert_native(p, Nest, {});
  auto h_exit = dict_insert_native(p, Unnest, {});

  const u16 spad = p->cb.here;
  p->cb.here += 32;
  NativeOpcode Word = {
      .stack_delta = 4,
      .name = "word",
      .h = [spad](Interpreter *i) { native_word(i, spad); },
  };
  auto h_word = dict_insert_native(p, Word, {}, "word");
  NativeOpcode WordBuffer = {
      .stack_delta = 4,
      .name = "&word",
      .h = [spad](Interpreter *i) { push_constant(i, spad); },
  };

  auto h_wordbuffer = dict_insert_native(p, WordBuffer, {}, "&word");
  auto h_create = dict_insert_native(p, Create, {});
  auto h_hidden = dict_insert_native(p, Hidden, {});
  auto h_latest = dict_insert_native(p, Latest, {});
  auto h_lit = dict_insert_native(p, Lit, {});
  auto h_fetch = dict_insert_native(p, Fetch, {}, "@");
  auto h_comma = dict_insert_native(p, Comma, {});
  auto h_lbrac = dict_insert_native(p, Lbrac, Flags::IMMEDIATE, "[");
  auto h_rbrac = dict_insert_native(p, Rbrac, {}, "]");

  u16 lit_pcode = h_lit.pcode();
  NativeOpcode Interp = {
      .stack_delta = 0,
      .name = "interpret",
      .h = [spad, lit_pcode](Interpreter *i) { native_interpret(i, spad, lit_pcode); },
  };
  auto h_interp = dict_insert_native(p, Interp, {}, "coreint");

  // "FORTH" words, implemented in terms of docol.
  // So early in the dictionary that we can't use :; ore true FORTH definitions yet.
  auto op_colon =
      std::array<u16, 10>{h_docol.code0(), h_word.pcode(),  h_create.pcode(), h_hidden.pcode(), h_lit.pcode(),
                          h_docol.pcode(), h_fetch.pcode(), h_comma.pcode(),  h_rbrac.pcode(),  h_exit.pcode()};
  dict_insert(p, ":", {}, op_colon);
  auto op_carrot = std::array<u16, 5>{h_docol.code0(), h_lit.pcode(), h_exit.pcode(), h_comma.pcode(), h_exit.pcode()};
  auto h_carrot = dict_insert(p, "^", Flags::IMMEDIATE, op_carrot);
  auto op_semi = std::array<u16, 8>{h_docol.code0(),  h_carrot.pcode(), h_latest.pcode(),
                                    h_hidden.pcode(), h_lbrac.pcode(),  h_exit.pcode()};
  dict_insert(p, ";", Flags::IMMEDIATE, op_semi);

  auto h_rspinitval = dict_insert_native(p, RspInitVal, {});
  auto h_rspstore = dict_insert_native(p, RspStoreVal, {});
  auto h_branch = dict_insert_native(p, Branch, {});

  auto op_quit = std::array<u16, 6>{h_docol.code0(),  h_rspinitval.pcode(), h_rspstore.pcode(),
                                    h_interp.pcode(), h_branch.pcode(),     (u16)-10};
  dict_insert(p, "quit", {}, op_quit);

  // Install the "quit" handler at the start point, which is address 0.
  NiceDictHeader h_quit = *dict_find(p, "quit");
  p->write(h_quit.pcode(), 0);
}

void register_common_words(Interpreter *p) {
  register_core_words(p);
  register_arithmetic_words(p);
  register_stack_words(p);
  register_memory_words(p);
  register_io_words(p);
  register_debug_words(p);
  register_sys_globals_words(p);
  register_dict_words(p);
  register_control_words(p);
}
