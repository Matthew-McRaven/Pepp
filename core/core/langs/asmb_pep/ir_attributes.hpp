#pragma once
#include <memory>
#include <string_view>
#include "core/arch/pepx/asmb/pep_mnemonics.hpp"
#include "core/arch/pepx/isa/pep_opcodes.hpp"
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

struct PepMnemonic : public AAttribute {
  static constexpr int TYPE = static_cast<int>(PepIRAttr::Mnemonic);
  int type() const override;
  PepMnemonic(std::string_view name, pep::MnemonicDescriptor mn) : name(name), mn(mn) {}
  // Points into the dialect's mnemonic table, so dialect must outlive this attribute.
  std::string_view name;
  pep::MnemonicDescriptor mn;
};

struct PepAddrMode : public AAttribute {
  static constexpr int TYPE = static_cast<int>(PepIRAttr::AddressingMode);
  int type() const override;
  PepAddrMode(pep::AddrMode mode) : addr_mode(mode) {}
  pep::AddrMode addr_mode = pep::AddrMode::INVALID;
};

// Intentionally NOT an AAttribute, because I do not want it stored in my primary IR.
// I want it stored in a side table
struct PeppAddress {
  using Type = u16;
  PeppAddress(u16 address, u16 size) : address(address), size(size) {}
  u16 address = 0, size = 0;
};

} // namespace pepp::tc
