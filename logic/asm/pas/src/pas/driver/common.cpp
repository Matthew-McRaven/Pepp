#include "./common.hpp"
#include "symbol/entry.hpp"
bool pas::driver::Globals::contains(QString symbol) const {
  return table.contains(symbol);
}

QSharedPointer<symbol::Entry> pas::driver::Globals::get(QString symbol) {
  return table[symbol];
}

bool pas::driver::Globals::add(QSharedPointer<symbol::Entry> symbol) {
  if (contains(symbol->name)) {
    symbol->state = symbol::DefinitionState::kExternalMultiple;
    get(symbol->name)->state = symbol::DefinitionState::kExternalMultiple;
    return false;
  } else
    table[symbol->name] = symbol;
  return true;
}
