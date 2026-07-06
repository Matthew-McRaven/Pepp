#include <algorithm>
#include <iostream>
#include "core/math/bitmanip/copy.hpp"
#include "core/math/bitmanip/enums.hpp"
#include "fmt/format.h"
#include "interp.hpp"

Interpreter::Interpreter() { std::fill(memory.begin(), memory.end(), 0); }

void Interpreter::step() {
  if (!cb.alive) return;
  cb.cur_ip = cb.nxt_ip;
  cb.nxt_ip += 2;
  auto opcode_indirect = read<u16>(cb.cur_ip);
  if (opcode_indirect >= memory.size()) {
    std::cerr << "**Opcode  out of bounds: " << opcode_indirect << std::endl;
    cb.alive = false;
    return;
  }
  cb.w = opcode_indirect;
  auto opcode_direct = read<u16>(opcode_indirect);
  if (opcode_direct >= memory.size()) {
    std::cerr << "*Opcode direct address out of bounds: " << opcode_direct << std::endl;
    cb.alive = false;
    return;
  }
  auto opcode = read<u16>(opcode_direct);
  std::cerr << fmt::format("cur_ip={:04x}, *cur_ip={:04x}, **cur_ip={:04x}, ***cur_ip={:04x}\n", cb.cur_ip,
                           opcode_indirect, opcode_direct, opcode);

  if (opcode == 0) {
    cb.alive = false;
    return;
  }
  auto it = native_words.find(opcode);
  if (it != native_words.end()) {
    it->second(this);
  } else {
    std::cerr << fmt::format("Unknown opcode **cur_ip={:x}\n", opcode);
    cb.alive = false;
  }
}

void Interpreter::run() {
  while (cb.alive) step();
}

u16 Interpreter::write(u16 base, std::span<const u8> data) {
  std::copy(data.begin(), data.end(), memory.begin() + base);
  return base + data.size();
}

u16 Interpreter::write(u16 base, std::span<const u16> data) {
  const bits::span<const u8> bytes((const u8 *)data.data(), data.size_bytes());
  return write(base, bytes);
}

u16 Interpreter::zeros(u16 base, u16 count) {
  std::fill(memory.begin() + base, memory.begin() + base + count, 0);
  return base + count;
}

u16 dict_insert(Interpreter *interp, std::string name, Flags flags, std::span<const u16> code, u16 codeword) {
  using namespace bits;
  static const u16 alignment = 2;
  const bool needs_null = name.ends_with("\0");
  auto *here = &interp->cb.here;
  // Add pading before string so that CFA will be aligned. Optionally 1 to enforce that all strings are null terminated.
  const auto unpadded_cfa = *here + (needs_null ? 1 : 0) + (u16)DictHeader::StaticOffsets::CODE + name.size();
  const u8 pad = (alignment - (unpadded_cfa % alignment)) % alignment;
  *here = interp->zeros(*here, pad);
  // Write out name
  *here = interp->write(*here, bits::span<const u8>{(const u8 *)name.data(), name.size()});
  // Add null terminator if source does not already include it.
  if (needs_null) *here = interp->zeros(*here, 1);

  // Write out backlink
  const u16 addr_of_link = *here;
  interp->write_here_pp(interp->cb.latest);

  // Compute len and combine with flags, accounting for masks & padding
  const u8 len = name.size() & (u8)Flags::MAX_LEN;
  const u8 with_flags = len | (u8)(flags & Flags::FLAG_MASK);
  interp->write_here_pp(with_flags);
  interp->write_here_pp<u8>(0);
  // Write out codeword
  if (codeword == 0) interp->write_here_pp<u16>(*here + 2);
  else interp->write_here_pp<u16>(codeword);
  // And write out any associated code.
  if (!code.empty()) interp->write_here_pp(code);

  interp->cb.latest = addr_of_link;

  return addr_of_link;
}

void native_add16i(Interpreter *interp) {
  i16 lhs = interp->pop_psp<i16>();
  i16 rhs = interp->pop_psp<i16>();
  i16 result = lhs + rhs;
  interp->push_psp(result);
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