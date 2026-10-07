#include "line_symbol.hpp"

pepp::tc::SymbolLine::SymbolLine(SymbolDeclaration symbol) : symbol(symbol) {}

const pepp::tc::AAttribute *pepp::tc::SymbolLine::attribute(int type) const {
  if (type == SymbolDeclaration::TYPE) return &symbol;
  else return LinearIR::attribute(type);
}

void pepp::tc::SymbolLine::insert(std::unique_ptr<AAttribute> attr) {
  if (attr->type() == SymbolDeclaration::TYPE) symbol = *static_cast<SymbolDeclaration *>(attr.get());
  else LinearIR::insert(std::move(attr));
}

int pepp::tc::SymbolLine::type() const { return TYPE; }
