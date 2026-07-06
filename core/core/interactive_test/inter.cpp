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
