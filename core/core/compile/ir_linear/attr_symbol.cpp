#include "attr_symbol.hpp"

pepp::tc::SymbolDeclaration::SymbolDeclaration(std::shared_ptr<core::symbol::Entry> entry) : entry(entry) {}

int pepp::tc::SymbolDeclaration::type() const { return TYPE; }

pepp::tc::LocationCounterDeclaration::LocationCounterDeclaration(std::shared_ptr<core::symbol::Entry> entry)
    : entry(std::move(entry)) {}

int pepp::tc::LocationCounterDeclaration::type() const { return TYPE; }
