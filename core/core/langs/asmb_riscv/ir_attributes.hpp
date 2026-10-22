/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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
  RISCVMnemonicAttribute(std::string_view name, riscv::MnemonicDescriptor mn);
  std::string_view name;
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
