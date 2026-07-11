#pragma once

#include <array>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <unordered_map>
#include "core/integers.h"
#include "core/math/bitmanip/copy.hpp"
#include "core/math/bitmanip/span.hpp"

// A simple forth-like interpreter. All "opcodes" are 16-bits.
// Negative opcodes are implemented directly in C++, while positive opcodes are implemented in terms of other opcodes.
// 0 is intentionally an illegal opcode.
// Unlike a normal Forth, there are three separate stacks. The classic data and return stacks still exist, with the
// addition of an object stack.

// memory layout
// 0x0000: initial value of ip, trampoline to real entry point
// 0x0010: initial value of here
// 0x0800: initial value of psp, growing towards 0x0FFF
// 0x0FFE: initial value of rsp, growing towards 0x0800
// OSP is not part of the memory map, because it is actually a vector of C++ objects.

class AValue;
class Interpreter;

struct NativeOpcode {
  // How many bytes should the stack have been adjusted by?
  i16 stack_delta;
  std::string name;
  using Handler = std::function<void(Interpreter *)>;
  Handler h;
};

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

struct AInput {
  virtual ~AInput() = default;
  virtual std::optional<std::string> readline() = 0;
  virtual bool has_input() const = 0;
};
struct NoInput : public AInput {
  std::optional<std::string> readline() override;
  bool has_input() const override { return false; }
};
struct StdinInput : public AInput {
  std::optional<std::string> readline() override;
  bool has_input() const override { return true; }
};

struct StringInput : public AInput {
  std::string input;
  size_t pos = 0;
  StringInput(std::string_view str) : input(str) {}
  std::optional<std::string> readline() override;
  bool has_input() const override { return pos < input.size(); }
};

struct AOutput {
  virtual ~AOutput() = default;
  virtual void write(std::string_view text) = 0;
};

struct StdoutOutput : public AOutput {
  void write(std::string_view text) override;
};

struct BufferedOutput : public AOutput {
  // All strings in buffer are terminated by a newline, with the possible exception of the last line.
  // This policy makes it much easier to write unit tests over the interpeter, which is the main purpose of this output
  // class.
  std::vector<std::string> buffer;
  // When text is inserted (via write), it is split on newlines and each line is appended to the buffer.
  // If the last line does not contain \n, future writes will append to that line until a \n is reached.
  void write(std::string_view text) override;
};

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
    bool alive = true, do_debug = false;
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
  // Excute the word at this IP next without modifying nxt_ip.
  std::optional<u16> redirect_next_step = std::nullopt;

  Interpreter();

  void step();
  void run();
  void run_on(std::string_view input);

  std::unordered_map<u16, NativeOpcode> native_words;
  void dispatch(u16 opcode);
  alignas(16) std::array<u8, 4096> memory;
  u16 write(u16 base, std::span<const u8> data);
  u16 write(u16 base, std::span<const u16> data);
  u16 zeros(u16 base, u16 count);
  bits::span<u8> memspan(u16 addr, u16 len) { return bits::span<u8>(memory.data() + addr, len); }
  bits::span<const u8> memspan(u16 addr, u16 len) const { return bits::span<const u8>(memory.data() + addr, len); }
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
  u16 register_native(NativeOpcode word) {
    i16 opcode = -((i16)native_words.size() + 1);
    native_words[opcode] = word;
    return opcode;
  }
  void get_input() {
    auto maybe_text = input_source->readline();
    if (maybe_text.has_value()) {
      storage = maybe_text.value();
      used_stdin = true;
    } else {
      storage.clear();
    }
  }
  void append_output(std::string text);
  std::string storage;
  std::string_view chars;
  std::unique_ptr<AInput> input_source{std::make_unique<NoInput>()};
  std::unique_ptr<AOutput> output{std::make_unique<StdoutOutput>()};
  bool used_stdin = false;
  bool has_input() const { return !chars.empty(); }

private:
  u16 _next_object_id = 1;
  std::map<u16, std::shared_ptr<AValue>> object_heap;

public:
  const std::map<u16, std::shared_ptr<AValue>> &get_object_heap() const { return object_heap; }
  u16 allocate_object(std::shared_ptr<AValue> obj) {
    auto id = _next_object_id++;
    object_heap[id] = obj;
    return id;
  }
  std::shared_ptr<AValue> get_object(u16 id) {
    auto it = object_heap.find(id);
    if (it != object_heap.end()) return it->second;
    return nullptr;
  }
};
