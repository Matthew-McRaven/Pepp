#pragma once
#include <bit>
#include <memory>
#include "core/arch/riscv/isa/rvi.hpp"
#include "core/integers.h"
namespace riscv {

union rv_instruction2 {
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
    auto copy = std::bit_cast<InstructionR>(*this);
    return copy.opcode;
  }
  template <typename T> inline T as() const noexcept { return std::bit_cast<T>(*this); }
  inline bool is_illegal() const noexcept { return low16() == 0x0000; }
  inline bool is_long() const noexcept { return (bits() & 0x3) == 0x3; }
  inline bool is_compressed() const noexcept { return (bits() & 0x3) != 0x3; }
  inline uint32_t length() const noexcept { return 2 + 2 * is_long(); }
  inline uint32_t fpfunc() const noexcept { return bits() >> 27; }
  inline uint32_t vwidth() const noexcept { return (bits() >> 12) & 0x7; }
  inline uint32_t vsetfunc() const noexcept { return bits() >> 30; }
};
static_assert(sizeof(rv_instruction2) == 4, "Instruction is 4 bytes");
} // namespace riscv
