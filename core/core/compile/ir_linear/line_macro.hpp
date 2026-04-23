#pragma once

#include <memory>
#include <vector>
#include "core/compile/ir_linear/line_base.hpp"

namespace pepp::tc {
struct MacroDefinition;
struct MacroInstantiation : public LinearIR {
  static constexpr int TYPE = static_cast<int>(LinearIRType::MacroInstantiation);
  explicit MacroInstantiation(std::shared_ptr<const MacroDefinition> d, std::vector<std::string> args);
  const AAttribute *attribute(int type) const override;
  void insert(std::unique_ptr<AAttribute> attr) override;
  int type() const override;
  // Do not expose via attibutes because I do not expect to do any pattern matching against this node.
  // I expoect usage will always check against the macro type and then static_cast.
  std::shared_ptr<const MacroDefinition> macro;
  std::vector<std::string> arguments;
  std::vector<std::shared_ptr<LinearIR>> lines;
};

struct InlineMacroDefinition : public LinearIR {
  static constexpr int TYPE = static_cast<int>(LinearIRType::MacroDefinition);
  explicit InlineMacroDefinition(std::string name, std::vector<std::string> args);
  const AAttribute *attribute(int type) const override;
  void insert(std::unique_ptr<AAttribute> attr) override;
  int type() const override;
  std::string name;
  std::vector<std::string> arguments;
  std::string body;
};
} // namespace pepp::tc
