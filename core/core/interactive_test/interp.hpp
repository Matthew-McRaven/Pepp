#pragma once

#include <array>
#include <functional>
#include <iostream>
#include <unordered_map>
#include "core/integers.h"
#include "core/math/bitmanip/copy.hpp"
#include "core/math/bitmanip/span.hpp"
class AValue {};

// A simple forth-like interpreter. All "opcodes" are 16-bits.
// Negative opcodes are implemented directly in C++, while positive opcodes are implemented in terms of other opcodes.
// 0 is intentionally an illegal opcode.
// Unlike a normal Forth, there are three separate stacks. The classic data and return stacks still exist, with the
// addition of an object stack.

// memory layout
// 0x0000: initial value of ip, trampoline to real entry point
// 0x0010: initial value of here
// 0x3000: initial value of psp, growing towards 0x4000
// 0x4000: initial value of rsp, growing towards 0x3000
// OSP is not part of the memory map, because it is actually a vector of C++ objects.

class Interpreter;

using NativeWord = std::function<void(Interpreter *)>;

enum class Flags : u8 {
  IMMEDIATE = 0x80,
  HIDDEN = 0x20,
  // LEN includes includes hidden bit.
  // This way, a hidden entry will NEVER match against a string, since
  // the reported length of the hidden word is longer than the max word size (31).
  // Trick borrowed from JoensForth.
  LEN = 0x3f,
  MAX_LEN = 0x1f, // Actual max len is 0x1f
  FLAG_MASK = 0xE0
};
consteval void is_bitflags(Flags);

class Interpreter {
public:
  enum class State : u8 {
    Compiling = 1,
    Immediate = 0,
  };
  // Pretend these are stored in registers.
  static const u16 INITIAL_PSP = 0x800;
  static const u16 INITIAL_RSP = 0xFFE;
  struct ControlBlock {
    bool alive = true;
    u8 state = 0x00; // interpreter state
    u8 _pad_ = 0;
    u16 cur_ip = 0x0000;   // Current codeword being executed.
    u16 nxt_ip = 0x0000;   // Next codeword to execute, equivalent to %esi in jonesforth.
    u16 w = 0x0000;        // cached pointer-to-pointer-to-opcode, equivalent to %eax in jonesforth
    u16 psp = INITIAL_PSP; // data stack pointer
    u16 rsp = INITIAL_RSP; // return stack pointer
    u16 osp = 0x0000;      // object stack pointer
    u16 here = 0x0010;     // next free memory location
    u16 latest = 0x0000;   // latest dictionary entry
  } cb{};

  Interpreter();

  void step();
  void run();

  std::unordered_map<u16, NativeWord> native_words;
  alignas(16) std::array<u8, 4096> memory;
  u16 write(u16 base, std::span<const u8> data);
  u16 write(u16 base, std::span<const u16> data);
  u16 zeros(u16 base, u16 count);
  template <std::integral I> void write(I value, u16 addr) {
    bits::span<const u8> src{reinterpret_cast<const u8 *>(&value), sizeof(I)};
    bits::span<u8> dest{memory.data() + addr, sizeof(I)};
    bits::memcpy(dest, src);
  }

  template <std::integral I> I read(u16 addr) const {
    I ret;
    bits::span<u8> dest{(u8 *)&ret, sizeof(I)};
    bits::span<const u8> src{memory.data(), memory.size()};
    src = src.subspan(addr, sizeof(I));
    bits::memcpy(dest, src);
    return ret;
  }
  template <std::integral I> void push_psp(I value) {
    write(value, cb.psp);
    cb.psp += sizeof(I);
  }
  template <std::integral I> I pop_psp() {
    if (cb.psp <= INITIAL_PSP) {
      std::cerr << "Data stack underflow!" << std::endl;
      cb.alive = false;
      return 0;
    }
    cb.psp -= sizeof(I);
    auto value = read<I>(cb.psp);
    return value;
  }
  template <std::integral I> void push_rsp(I value) {
    if (cb.rsp >= memory.size()) {
      std::cerr << "Overflow";
    }
    write(value, cb.rsp);
    cb.rsp -= sizeof(I);
  }
  template <std::integral I> I pop_rsp() {
    if (cb.rsp >= INITIAL_RSP) {
      std::cerr << "Return stack underflow!" << std::endl;
      cb.alive = false;
      return 0;
    }
    cb.rsp += sizeof(I);
    auto value = read<I>(cb.rsp);
    return value;
  }
  template <std::integral I> void write_here_pp(I v) {
    write(v, cb.here);
    cb.here += sizeof(I);
  }
  void write_here_pp(std::span<const u8> data) { cb.here = write(cb.here, data); }
  void write_here_pp(std::span<const u16> code) {
    for (auto c : code) write_here_pp<u16>(c);
  }
  u16 register_native(NativeWord word) {
    i16 opcode = -((i16)native_words.size() + 1);
    native_words[opcode] = word;
    return opcode;
  }
  std::string storage;
  std::string_view chars;
};
