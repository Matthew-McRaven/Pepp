#include "core_words.hpp"
#include <charconv>
#include "./interp.hpp"
#include "core/integers.h"
#include "core/interactive_test/dict.hpp"
#include "fmt/format.h"

void native_add16i(Interpreter *interp) {
  i16 lhs = interp->pop_psp<i16>();
  i16 rhs = interp->pop_psp<i16>();
  i16 result = lhs + rhs;
  interp->push_psp(result);
}

void native_lit(Interpreter *interp) {
  u16 *nxt_ip = &interp->cb.nxt_ip;
  u16 value = interp->read<u16>(*nxt_ip);
  *nxt_ip += 2;
  interp->push_psp(value);
}

void native_dup16(Interpreter *interp) {
  i16 value = interp->pop_psp<i16>();
  interp->push_psp(value);
  interp->push_psp(value);
}

void native_docol(Interpreter *interp) {
  // Push next ip onto return stack
  interp->push_rsp(interp->cb.nxt_ip);
  interp->cb.nxt_ip = interp->cb.w + 2;
}

void native_exitcol(Interpreter *interp) {
  // Pop top of RSP into next ip.
  interp->cb.nxt_ip = interp->pop_rsp<u16>();
}

void native_rspinitval(Interpreter *interp) { interp->push_psp(Interpreter::INITIAL_RSP); }

void native_rspstoreval(Interpreter *interp) { interp->cb.rsp = interp->pop_psp<u16>(); }

void native_psp(Interpreter *interp) { interp->push_psp(interp->cb.psp); }

void native_rsp(Interpreter *interp) { interp->push_psp(interp->cb.rsp); }

void native_halt(Interpreter *interp) { interp->cb.alive = false; }
void native_latest(Interpreter *interp) { interp->push_psp(interp->cb.latest); }

void native_here(Interpreter *interp) { interp->push_psp(interp->cb.here); }
void native_fetch(Interpreter *interp) {
  i16 addr = interp->pop_psp<i16>();
  i16 value = interp->read<i16>(addr);
  interp->push_psp(value);
}

void native_hidden(Interpreter *interp) {
  // std::cerr << "Hidden is unimplemented"
  u16 nt_addr = interp->pop_psp<u16>();
  auto hdr = NiceDictHeader(interp, nt_addr);
  hdr.toggle_hidden();
}

void native_swap16(Interpreter *interp) {
  i16 first = interp->pop_psp<i16>();
  i16 second = interp->pop_psp<i16>();
  interp->push_psp(first);
  interp->push_psp(second);
}

void native_drop16(Interpreter *interp) { i16 value = interp->pop_psp<i16>(); }

void native_over16(Interpreter *interp) {
  i16 first = interp->pop_psp<i16>();
  i16 second = interp->pop_psp<i16>();
  interp->push_psp(second);
  interp->push_psp(first);
  interp->push_psp(second);
}

void native_dot(Interpreter *interp) {
  auto tos = interp->pop_psp<i16>();
  std::cout << tos << std::endl;
}

void native_key(Interpreter *interp) {
  // If our span of chararacters is empty, fgetch more input!
  if (interp->chars.empty()) {
    // Prefer to take input from the array of buffered strings
    if (!interp->buffered.empty()) {
      interp->storage = interp->buffered.front();
      interp->buffered.erase(interp->buffered.begin());

    } else { // Otherwise resort to stdin
      std::getline(std::cin, interp->storage);
      interp->used_stdin = true;
    }
    // Ensure content is newline terminated
    if (!interp->storage.ends_with('\n')) interp->storage += '\n';
    interp->chars = interp->storage;
  }
  // If buffer is empty, read all available text from stdin into the interpreter's buffer

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

void native_emit(Interpreter *interp) {
  const auto c = interp->pop_psp<char>();
  std::cout << c;
}

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
    if (c == 0 || std::isspace(c)) {
      // Null terminate the string and return

      break;
    } else {
      // Write character to buffer
      interp->write<char>(c, buffer_addr++);
    }
  }
  // Ensure word is null terminated
  interp->write<char>(0, buffer_addr++);
  u16 size = buffer_addr - initial - 1; // Exclude null terminator
  return size;
}
void native_word(Interpreter *interp, u16 buffer_addr) {
  auto size = word_helper(interp, buffer_addr);
  interp->push_psp(buffer_addr);
  interp->push_psp(size);
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
void native_number(Interpreter *interp) {
  u16 size = interp->pop_psp<u16>();
  u16 addr = interp->pop_psp<u16>();
  auto res = number_helper(interp, addr, size);
  if (res.has_value()) interp->push_psp((i16)res.value());
  else interp->push_psp((i16)0);
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

void native_find(Interpreter *interp) {
  DictionaryIterator iter(interp);
  const auto end = DictionaryIterator(interp, 0);
  u16 size = interp->pop_psp<u16>();
  u16 addr = interp->pop_psp<u16>();
  u16 ret = find_helper(interp, addr, size);
  interp->push_psp(ret);
}

void native_cfa(Interpreter *interp) {
  u16 nt_addr = interp->pop_psp<u16>();
  NiceDictHeader hdr(interp, nt_addr);
  interp->push_psp(hdr.pcode());
}

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

void native_comma(Interpreter *interp) {
  u16 value = interp->pop_psp<u16>();
  interp->write_here_pp<u16>(value);
}

void native_lbrac(Interpreter *interp) { interp->cb.state = (u8)Interpreter::State::Immediate; }

void native_rbrac(Interpreter *interp) { interp->cb.state = (u8)Interpreter::State::Compiling; }

void native_branch(Interpreter *interp) {
  // Read offset from next slot after IP
  u16 *nxt_ip = &interp->cb.nxt_ip;
  u16 value = interp->read<u16>(*nxt_ip);
  // Increment by +2 to account for the read of the next slot.
  *nxt_ip = *nxt_ip + value + 2;
}

void native_zbranch(Interpreter *interp) {
  // If TOS is 0, take the branch.
  if (i16 tos = interp->pop_psp<i16>(); tos == 0) native_branch(interp);
  // Otherwise consume the offset without branching.
  else interp->cb.nxt_ip += 2;
}

void native_interpret(Interpreter *interp, u16 word_buffer, u16 pcode_lit) {
  auto size = word_helper(interp, word_buffer);
  auto nt_addr = find_helper(interp, word_buffer, size);
  std::string_view word_buffer_str(reinterpret_cast<const char *>(interp->memory.data() + word_buffer), size);

  // Not found, try to parse as a number.
  if (nt_addr == 0) {
    auto num = number_helper(interp, word_buffer, size);
    // If compiling, must comple as a literal.
    if (!num.has_value()) {
      // Force simulator to go back to entry point.
      interp->cb.nxt_ip = 0;
      std::cout << word_buffer_str << " ?" << std::endl;
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
  if (interp->cb.state == (u8)Interpreter::State::Immediate && !interp->has_input() && interp->used_stdin)
    std::cout << "  ok\n";
}

void native_lateststore(Interpreter *interp) {
  u16 value = interp->pop_psp<u16>();
  interp->cb.latest = value;
}

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

void native_toggle_debug(Interpreter *interp) { interp->cb.do_debug = !interp->cb.do_debug; }

void register_core_words(Interpreter *p) {
  // Words with automatic registration.
  auto h_stop = dict_insert_native(p, Halt, {});
  auto h_dup = dict_insert_native(p, Dup16, {});
  auto h_drop = dict_insert_native(p, Drop16, {});
  auto h_add16i = dict_insert_native(p, Add16i, {}, "+");
  auto h_dot = dict_insert_native(p, Dot, {}, ".");
  auto h_docol = dict_insert_native(p, Docol, {}, "docol");
  auto h_exit = dict_insert_native(p, Exitcol, {});
  auto h_create = dict_insert_native(p, Create, {});
  auto h_comma = dict_insert_native(p, Comma, {});
  auto h_lit = dict_insert_native(p, Lit, {});
  auto h_latest = dict_insert_native(p, Latest, {});
  auto h_here = dict_insert_native(p, Here, {});
  auto h_fetch = dict_insert_native(p, Fetch, {}, "@");
  auto h_hidden = dict_insert_native(p, Hidden, {});
  auto h_rbrac = dict_insert_native(p, Rbrac, {}, "]");
  auto h_lbrac = dict_insert_native(p, Lbrac, Flags::IMMEDIATE, "[");
  auto h_branch = dict_insert_native(p, Branch, {});
  auto h_lateststore = dict_insert_native(p, LatestStore, {}, "latest!");
  auto h_rspinitval = dict_insert_native(p, RspInitVal, {});
  auto h_rspstore = dict_insert_native(p, RspStoreVal, {});
  auto h_psp = dict_insert_native(p, PspVal, {});
  auto h_rsp = dict_insert_native(p, RspVal, {});
  auto h_dumpdict = dict_insert_native(p, DumpDict, {});
  auto h_toggledebug = dict_insert_native(p, ToggleDebug, {});
  auto h_cmove = dict_insert_native(p, CMove, {});
  auto h_cmove0 = dict_insert_native(p, CMove0, {});
  auto h_print0 = dict_insert_native(p, PrintNullTerminated, {});

  // Word which require per-instance state.
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
  u16 lit_pcode = h_lit.pcode();
  NativeOpcode Interp = {
      .stack_delta = 0,
      .name = "interpret",
      .h = [spad, lit_pcode](Interpreter *i) { native_interpret(i, spad, lit_pcode); },
  };
  auto h_interp = dict_insert_native(p, Interp, {}, "interpret");

  // "FORTH" words, implemented in terms of docol
  auto op_colon =
      std::array<u16, 10>{h_docol.code0(), h_word.pcode(),  h_create.pcode(), h_hidden.pcode(), h_lit.pcode(),
                          h_docol.pcode(), h_fetch.pcode(), h_comma.pcode(),  h_rbrac.pcode(),  h_exit.pcode()};
  auto d_colon = dict_insert(p, ":", {}, op_colon);
  auto op_semi = std::array<u16, 8>{h_docol.code0(),  h_lit.pcode(),    h_exit.pcode(),  h_comma.pcode(),
                                    h_latest.pcode(), h_hidden.pcode(), h_lbrac.pcode(), h_exit.pcode()};
  auto d_semi = dict_insert(p, ";", Flags::IMMEDIATE, op_semi);

  auto op_quit = std::array<u16, 6>{h_docol.code0(),  h_rspinitval.pcode(), h_rspstore.pcode(),
                                    h_interp.pcode(), h_branch.pcode(),     (u16)-10};
  auto d_quit = dict_insert(p, "quit", {}, op_quit);
}

void native_cmove(Interpreter *interp) {
  u16 dst = interp->pop_psp<u16>();
  u16 size = interp->pop_psp<u16>();
  u16 src = interp->pop_psp<u16>();
  auto src_span = std::span<const u8>(interp->memory.data() + src, size);
  interp->write(dst, src_span);
}

void native_cmove0(Interpreter *interp) {
  u16 dst = interp->pop_psp<u16>();
  u16 size = interp->pop_psp<u16>();
  u16 src = interp->pop_psp<u16>();
  auto src_span = std::span<const u8>(interp->memory.data() + src, size);
  interp->write(dst, src_span);
  u16 null_terminator_addr = dst + size;
  interp->write<u8>(0, null_terminator_addr);
}
void native_print_nullterminated(Interpreter *interp) {
  u16 addr = interp->pop_psp<u16>();
  while (true) {
    u8 c = interp->read<u8>(addr++);
    if (c == 0) break;
    std::cout << static_cast<char>(c);
  }
}

void push_constant(Interpreter *interp, u16 addr) { interp->push_psp(addr); }
