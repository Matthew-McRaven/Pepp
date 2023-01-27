#include "entry.hpp"
symbol::Entry::Entry(symbol::Table &parent, QString name)
    : parent(parent), state(DefinitionState::kUndefined), name(name),
      binding(Binding::kLocal),
      value(QSharedPointer<symbol::value::Empty>::create(0)) {}
