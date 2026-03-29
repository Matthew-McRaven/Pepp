#pragma once
#include <memory>
#include <string>
#include "core/arch/pep/isa/pep10.hpp"
#include "core/compile/ir_linear/attr_base.hpp"
#include "core/integers.h"

namespace pepp::ast {
class IRValue;
}
namespace pepp::core::symbol {
class Entry;
}
namespace pepp::tc {
enum class PepIRAttr {
  Mnemonic = static_cast<int>(Type::FirstUser),
  AddressingMode,
};

struct Pep10Mnemonic : public AAttribute {
  static constexpr int TYPE = static_cast<int>(PepIRAttr::Mnemonic);
  int type() const override;
  Pep10Mnemonic(isa::Pep10::Mnemonic instruction) : instruction(instruction) {}
  isa::Pep10::Mnemonic instruction = isa::Pep10::Mnemonic::INVALID;
};

struct Pep10AddrMode : public AAttribute {
  static constexpr int TYPE = static_cast<int>(PepIRAttr::AddressingMode);
  int type() const override;
  Pep10AddrMode(isa::Pep10::AddressingMode mode) : addr_mode(mode) {}
  isa::Pep10::AddressingMode addr_mode = isa::Pep10::AddressingMode::INVALID;
};

// Intentionally NOT an AAttribute, because I do not want it stored in my primary IR.
// I want it stored in a side table
struct Address {
  Address(u16 address, u16 size) : address(address), size(size) {}
  u16 address = 0, size = 0;
};

} // namespace pepp::tc
