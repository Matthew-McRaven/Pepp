#pragma once

#include "core/compile/ir_linear/attr_base.hpp"

namespace pepp::ast {
class IRValue;
}
namespace pepp::tc {
struct Argument : public AAttribute {
  static constexpr int TYPE = static_cast<int>(Type::Argument);
  int type() const override;
  explicit Argument(std::shared_ptr<pepp::ast::IRValue> value);
  std::shared_ptr<pepp::ast::IRValue> value;
};
} // namespace pepp::tc
