#include "core_words.hpp"
#include <charconv>
#include "./interp.hpp"
#include "core/integers.h"
#include "core/interactive_test/dict.hpp"

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

void native_halt(Interpreter *interp) { interp->cb.alive = false; }
void native_latest(Interpreter *interp) { interp->push_psp(interp->cb.latest); }
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
  // If buffer is empty, read all available text from stdin into the interpreter's buffer
  if (interp->chars.empty()) {
    std::getline(std::cin, interp->storage);
    interp->storage += '\n'; // Add a newline to simulate pressing enter
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
  if (ec != std::errc() || ptr != sub_end) {
    std::cerr << "Not a number: " << str;
    return std::nullopt;
  }
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

  // Not found, try to parse as a number.
  if (nt_addr == 0) {
    auto num = number_helper(interp, word_buffer, size);
    // If compiling, must comple as a literal.
    if (!num.has_value()) {
      std::cerr << "Not a number: " << std::string_view((const char *)interp->memory.data() + word_buffer, size)
                << std::endl;
    } else if (interp->cb.state == (u8)Interpreter::State::Compiling) {
      interp->write_here_pp(pcode_lit);
      interp->write_here_pp<u16>(num.value_or(0));
    } else {
      interp->push_psp<u16>(num.value_or(0));
    }
    return;
  }

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

void native_lateststore(Interpreter *interp) {
  u16 value = interp->pop_psp<u16>();
  interp->cb.latest = value;
}
