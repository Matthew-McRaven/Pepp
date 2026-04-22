#include "line_macro.hpp"

pepp::tc::MacroInstantiation::MacroInstantiation(std::shared_ptr<const MacroDefinition> d, std::vector<std::string> args)
    : macro(std::move(d)), arguments(args) {}

void pepp::tc::MacroInstantiation::insert(std::unique_ptr<AAttribute> attr) { LinearIR::insert(std::move(attr)); }

int pepp::tc::MacroInstantiation::type() const { return TYPE; }

const pepp::tc::AAttribute *pepp::tc::MacroInstantiation::attribute(int type) const { return LinearIR::attribute(type); }

pepp::tc::InlineMacroDefinition::InlineMacroDefinition(std::string name, std::vector<std::string> args)
    : name(name), arguments(args) {}

void pepp::tc::InlineMacroDefinition::insert(std::unique_ptr<AAttribute> attr) { LinearIR::insert(std::move(attr)); }

int pepp::tc::InlineMacroDefinition::type() const { return TYPE; }

const pepp::tc::AAttribute *pepp::tc::InlineMacroDefinition::attribute(int type) const {
  return LinearIR::attribute(type);
}
