#pragma once

#include "core/compile/ir_linear/attr_base.hpp"
namespace pepp::core::symbol {
class Entry;
}
namespace pepp::tc {
struct SymbolDeclaration : public AAttribute {
  static constexpr int TYPE = static_cast<int>(Type::SymbolDeclaration);
  int type() const override;
  explicit SymbolDeclaration(std::shared_ptr<pepp::core::symbol::Entry> entry);
  std::shared_ptr<pepp::core::symbol::Entry> entry;
};
} // namespace pepp::tc
