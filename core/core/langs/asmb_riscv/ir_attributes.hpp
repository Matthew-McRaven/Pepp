#pragma once
#include <memory>
#include <string>
#include "core/arch/riscv/asmb/rv_mnemonics.hpp"
#include "core/compile/ir_linear/attr_base.hpp"

namespace pepp::tc {
enum class RISCVIRAttr {
  Mnemonic = static_cast<int>(Type::FirstUser),
};

struct RISCVMnemonicAttribute : public AAttribute {
  static constexpr int TYPE = static_cast<int>(RISCVIRAttr::Mnemonic);
  int type() const override;
  RISCVMnemonicAttribute(riscv::MnemonicDescriptor mn);
  riscv::MnemonicDescriptor mn;
};

// Intentionally NOT an AAttribute, because I do not want it stored in my primary IR.
// I want it stored in a side table
struct RISCVAddress {
  using Type = u32;
  RISCVAddress(u32 address, u32 size) : address(address), size(size) {}
  u32 address = 0, size = 0;
};

} // namespace pepp::tc
