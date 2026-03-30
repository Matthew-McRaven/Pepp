#pragma once

#include <string>
#include "core/compile/ir_linear/attr_base.hpp"

namespace pepp::tc {

struct Identifier : public AAttribute {
  static constexpr int TYPE = static_cast<int>(Type::Identifier);
  int type() const override;
  explicit Identifier(std::string v);
  std::string value;
  std::string_view view() const;
};
} // namespace pepp::tc
