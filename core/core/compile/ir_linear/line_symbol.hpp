#pragma once

#include "core/compile/ir_linear/attr_symbol.hpp"
#include "core/compile/ir_linear/line_base.hpp"

namespace pepp::tc {

struct SymbolLine : public LinearIR {
  static constexpr int TYPE = static_cast<int>(LinearIRType::Symbol);
  SymbolLine(SymbolDeclaration symbol);
  const AAttribute *attribute(int type) const override;
  void insert(std::unique_ptr<AAttribute> attr) override;
  int type() const override;
  SymbolDeclaration symbol;
  // When non-nullptr, this line's symbol declaration should mirror the address+type+size of the pointed-to line. When
  // nullptr, this line is after the last code-generating line in the section and needs special handling to determine
  // its address+type+size. Assigned during assign_addresses.
  const LinearIR *target = nullptr;
};

} // namespace pepp::tc
