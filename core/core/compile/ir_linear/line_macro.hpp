#pragma once

#include "core/compile/ir_linear/line_base.hpp"
namespace pepp::tc {
struct MacroDefinition;
struct MacroLine : public LinearIR {
  static constexpr int TYPE = static_cast<int>(LinearIRType::Macro);
  explicit MacroLine(std::shared_ptr<const MacroDefinition> d, std::vector<std::string> args);
  const AAttribute *attribute(int type) const override;
  void insert(std::unique_ptr<AAttribute> attr) override;
  int type() const override;
  // Do not expose via attibutes because I do not expect to do any pattern matching against this node.
  // I expoect usage will always check against the macro type and then static_cast.
  std::shared_ptr<const MacroDefinition> macro;
  std::vector<std::string> arguments;
};
} // namespace pepp::tc
