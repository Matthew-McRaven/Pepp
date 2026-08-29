#pragma once
#include <bit>
#include <string>
#include "core/arch/riscv/isa/rv_instruction_list.hpp"
#include "core/integers.h"
namespace riscv {

union rv_instruction2 {
  struct ExtractOpcode {
    u32 opcode : 7;
    u32 rest : 25;
  };

  u8 bytes[4];
  u16 half[2];
  u32 whole;

  constexpr rv_instruction2() : whole(0) {}
  constexpr rv_instruction2(u32 other) : whole(other) {}
  template <typename T> constexpr rv_instruction2(T other) : rv_instruction2(std::bit_cast<u32>(other)) {}
  inline uint32_t bits() const noexcept { return std::bit_cast<uint32_t>(*this); }
  inline uint16_t low16() const noexcept { return static_cast<uint16_t>(bits()); }
  inline uint16_t high16() const noexcept { return static_cast<uint16_t>(bits() >> 16); }
  inline uint32_t opcode() const noexcept {
    auto copy = std::bit_cast<ExtractOpcode>(*this);
    return copy.opcode;
  }
  // Opcode if a compressed instruction.
  inline uint16_t copcode() const noexcept { return static_cast<uint16_t>(low16() & 0b1110000000000011); }
  template <typename T> inline T as() const noexcept { return std::bit_cast<T>(*this); }
  // Dissassemble by decode()ing and then calling the correct instruction formatting helper.
  std::string to_string() const;
  // Compressed instructions occupy the low half, and bit_cast requires source, dest to match in size.
  // Therefore we need to select the low half explicitly.
  template <typename T> inline T as_compressed() const noexcept { return std::bit_cast<T>(low16()); }

  inline bool is_illegal() const noexcept { return low16() == 0x0000; }
  inline bool is_long() const noexcept { return (bits() & 0x3) == 0x3; }
  inline bool is_compressed() const noexcept { return (bits() & 0x3) != 0x3; }
  inline uint32_t length() const noexcept { return 2 + 2 * is_long(); }
  inline uint32_t fpfunc() const noexcept { return bits() >> 27; }
  inline uint32_t vwidth() const noexcept { return (bits() >> 12) & 0x7; }
  inline uint32_t vsetfunc() const noexcept { return bits() >> 30; }
};
static_assert(sizeof(rv_instruction2) == 4, "Instruction is 4 bytes");

// Decode one instruction to its abstract opcode. Returns RvOp::INVALID for anything RV32I does not define.
RvOp decode(rv_instruction2 w) noexcept;
} // namespace riscv
