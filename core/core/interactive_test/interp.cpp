#include <algorithm>
#include <iostream>
#include <string>
#include "core/math/bitmanip/copy.hpp"
#include "core/math/bitmanip/enums.hpp"
#include "fmt/format.h"
#include "fmt/ranges.h"
#include "interp.hpp"

std::optional<std::string> StdinInput::readline() {
  std::string ret;
  if (!std::getline(std::cin, ret)) return std::nullopt;
  return ret;
}

std::optional<std::string> NoInput::readline() { return std::nullopt; }

std::optional<std::string> StringInput::readline() {
  if (input.empty()) return std::nullopt;
  auto end = input.find_first_of("\n", pos);
  std::string ret = input.substr(pos, end - pos);
  pos = (end == std::string::npos) ? input.size() : end + 1;
  return ret;
}
Interpreter::Interpreter() { std::fill(memory.begin(), memory.end(), 0); }

void Interpreter::step() {
  if (!cb.alive) return;

  u16 opcode = 0;
  if (redirect_next_step.has_value()) {
    redirect_next_step = std::nullopt;
    cb.w = *redirect_next_step;
  } else {
    cb.cur_ip = cb.nxt_ip;
    redirect_next_step = std::nullopt;
    cb.nxt_ip += 2;
    cb.w = read<u16>(cb.cur_ip);
  }

  if (cb.w >= memory.size()) {
    std::cerr << "*Opcode direct address out of bounds: " << cb.w << std::endl;
    cb.alive = false;
    return;
  } else opcode = read<u16>(cb.w);

  if (opcode == 0) {
    cb.alive = false;
    return;
  }
  dispatch(opcode);
}

void Interpreter::run() {
  while (cb.alive) step();
}

void Interpreter::run_on(std::string_view input) {
  auto old_alive = cb.alive;
  auto old_input = std::move(input_source);
  input_source = std::make_unique<StringInput>(std::string(input));
  while (cb.alive) step();
  cb.alive = old_alive;
  input_source = std::move(old_input);
}

void Interpreter::dispatch(u16 opcode) {
  auto it = native_words.find(opcode);
  if (it != native_words.end()) {
    if (cb.do_debug) {
      std::cerr << fmt::format("{:9}, cur_ip={:04x}, *cur_ip={:04x}, **cur_ip={:04x}\n", it->second.name, cb.cur_ip,
                               cb.w, opcode);
      std::cerr << fmt::format("   state={:1x},psp={:04x}, rsp={:04x}, here={:04x}, latest={:04x}\n", (i16)cb.state,
                               cb.psp, cb.rsp, cb.here, cb.latest);
    }
    const auto init_psp = cb.psp;
    it->second.h(this);
    const auto final_psp = cb.psp;
    auto delta = final_psp - init_psp;
    if (cb.do_debug) {
      if (delta != it->second.stack_delta)
        std::cerr << fmt::format("  warning: native word stack delta mismatch. Expected {}, got {}.\n",
                                 it->second.stack_delta, delta);
      std::cerr << fmt::format("   state={:1x},psp={:04x}, rsp={:04x}, here={:04x}, latest={:04x}\n", (i16)cb.state,
                               cb.psp, cb.rsp, cb.here, cb.latest);
    }
  } else {
    std::cerr << fmt::format(". Unknown opcode **cur_ip={:x}\n", opcode);
    cb.alive = false;
  }
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

void Interpreter::append_output(std::string text) { output->write(text); }
void StdoutOutput::write(std::string_view text) { std::cout << text; }
void BufferedOutput::write(std::string_view text) {
  if (text.empty()) return;

  size_t start = 0;
  while (start < text.size()) {
    size_t pos = text.find('\n', start);
    // Include the newline in the segment; if none found, take the rest (partial line)
    size_t end = (pos == std::string_view::npos) ? text.size() : pos + 1;
    std::string_view segment = text.substr(start, end - start);
    // Prefer to complete the last line in the buffer if it does not contain a newline. Otherwise, start a newline.
    if (!buffer.empty() && buffer.back().back() != '\n') buffer.back().append(segment);
    else buffer.push_back(std::string(segment));
    start = end;
  }
}
