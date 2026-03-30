#pragma once
#include <memory>
#include <optional>
#include "core/compile/ir_linear/line_base.hpp"

namespace pepp::tc {
struct EmptyLine : public LinearIR {
  static constexpr int TYPE = static_cast<int>(LinearIRType::Empty);
  int type() const override;
};
} // namespace pepp::tc
