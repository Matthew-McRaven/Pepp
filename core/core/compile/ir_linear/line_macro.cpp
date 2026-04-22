#include "line_macro.hpp"

pepp::tc::MacroLine::MacroLine(std::shared_ptr<const MacroDefinition> d, std::vector<std::string> args)
    : macro(std::move(d)), arguments(args) {}

void pepp::tc::MacroLine::insert(std::unique_ptr<AAttribute> attr) { LinearIR::insert(std::move(attr)); }

int pepp::tc::MacroLine::type() const { return TYPE; }

const pepp::tc::AAttribute *pepp::tc::MacroLine::attribute(int type) const { return LinearIR::attribute(type); }
