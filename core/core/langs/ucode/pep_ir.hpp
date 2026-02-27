#pragma once
#include <fmt/format.h>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>
#include "core/integers.h"

namespace pepp::tc::ir {
// A test condition which gets/sets a memory address
struct MemTest {
  MemTest() = default;
  MemTest(u16 addr, u16 value);
  MemTest(u16 addr, u8 value);
  ~MemTest() = default;
  MemTest(const MemTest &) = default;
  MemTest(MemTest &&) = default;
  MemTest &operator=(const MemTest &) = default;
  MemTest &operator=(MemTest &&) = default;
  u8 value[2] = {0, 0};
  u16 address = 0;
  u8 size = 1; // Either 1 or 2 bytes.
  operator std::string() const;
};
// A test condition which gets/sets a variable-sized register
template <typename registers> struct RegisterTest {
  typename registers::NamedRegisters reg;
  // Based on the size of the register, you MUST mask this down to [1-3] bytes.
  // You will also need to byteswap and shift depending on the architecture
  u32 value = 0;
  operator std::string() const {
    const auto size = registers::register_byte_size(reg);
    static const u32 masks[4] = {0, 0xFF, 0xFF'FF, 0xFF'FF'FF'FF};
    return fmt::format("{}=0x{:0{}X}", registers::register_name(reg), value & masks[size], 2 * size);
  }
};
// A test condition which gets/sets a variable-sized register
template <typename registers> struct CSRTest {
  typename registers::CSRs reg;
  bool value = false;
  operator std::string() const { return fmt::format("{}={}", registers::csr_name(reg), (int)value); };
};

// A UnitPre or UnitPost is a comma-delimited set of memory tests or register tests
template <typename registers> using Test = std::variant<MemTest, RegisterTest<registers>, CSRTest<registers>>;

// Wrap control signals (e.g., our object code) with some assembler IR
template <typename uarch, typename registers> struct Line {
  typename uarch::CodeWithEnables controls;
  std::optional<std::string> comment, symbolDecl;
  u16 address = -1; // Needed to compute branch targets in control section
  // Some signals allow symbolic values, whose values are only known after parsing completes.
  // This records which signals must be updated after all lines have been parsed.
  std::map<typename uarch::Signals, std::string> deferredValues;
  // Will always be code unless this is a UnitPre or UnitPost.
  enum class Type { Code, Pre, Post } type = Type::Code;
  // List of all tests defined in UnitPre/UnitPost. See type to determine which.
  std::vector<Test<registers>> tests;
};

} // namespace pepp::tc::ir
