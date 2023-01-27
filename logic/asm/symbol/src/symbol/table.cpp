#include "table.hpp"
#include "visit.hpp"
symbol::Table::Table(QSharedPointer<Table> parent) : parent(parent) {}

QSharedPointer<symbol::Table> symbol::Table::addChild() {
  auto child = QSharedPointer<symbol::Table>::create(this->sharedFromThis());
  _children.push_back(child);
  return child;
}

QList<QSharedPointer<symbol::Table>> symbol::Table::children() {
  return _children;
}

std::optional<symbol::Table::entry_ptr_t>
symbol::Table::get(const QString &name) const {
  if (auto item = _name_to_entry.find(name); item != _name_to_entry.end())
    return item.value();
  else
    return std::nullopt;
}

symbol::Table::entry_ptr_t symbol::Table::reference(const QString &name) {
  // Create a local definition if one does not already exist
  entry_ptr_t local_definition;
  if (auto pair = _name_to_entry.find(name); pair == _name_to_entry.end()) {
    //  Symbol is  new, just add to map
    local_definition = QSharedPointer<symbol::Entry>::create(*this, name);
    _name_to_entry[name] = local_definition;
  } else {
    //  Get pointer to existing definition
    local_definition = pair.value();
  }

  // Check for the presence of other symbols with the same name
  auto as_ptr = this->sharedFromThis();
  auto symbols =
      symbol::selectByName(as_ptr, name, TraversalPolicy::kWholeTree);
  int global_count = 0;
  for (auto &symbol : symbols) {
    if (&symbol->parent == &*this)
      continue; // We will be examining our symbols later.
    global_count += (symbol->binding == symbol::Binding::kGlobal) ? 1 : 0;
  }

  // If there's more than one global definition, we already know that the
  // program is invalid.
  if (global_count > 1) {
    local_definition->state = symbol::DefinitionState::kExternalMultiple;
    //  Local variable should be treated as global if name is already in global
    //  space
    local_definition->binding = symbol::Binding::kGlobal;
    // If there's one definition, take on most of the properties of that
    // definition
  } else if (global_count == 1) {
    for (auto &other : symbols) {
      if (other->binding == symbol::Binding::kGlobal) {
        local_definition->value =
            QSharedPointer<symbol::value::Pointer>::create(other);
        // Mark the symbol as imported, so that we can tell the difference
        // between our global symbols and others' globals.
        local_definition->binding = symbol::Binding::kImported;
        local_definition->state = other->state;
      }
    }
  }
  return local_definition;
}

symbol::Table::entry_ptr_t symbol::Table::define(const QString &name) {
  auto entry = reference(name);

  // Check for the presence of other symbols with the same name
  auto as_ptr = this->sharedFromThis();
  auto same_name =
      symbol::selectByName(as_ptr, name, TraversalPolicy::kWholeTree);

  switch (entry->binding) {
  case symbol::Binding::kImported:
    entry->state = DefinitionState::kExternalMultiple;
    break;
  case symbol::Binding::kGlobal:
    if (entry->state == DefinitionState::kUndefined)
      entry->state = DefinitionState::kSingle;
    else if (entry->state == DefinitionState::kSingle)
      entry->state = DefinitionState::kMultiple;

    for (auto other : same_name) {
      if (&other->parent == &*this)
        continue;
      else if (other->binding == symbol::Binding::kImported) {
        other->value = QSharedPointer<symbol::value::Pointer>::create(entry);
        // Mark the symbol as imported, so that we can tell the difference
        // between our global symbols and others' globals.
        other->binding = symbol::Binding::kImported;
        other->state = entry->state;
      }
    }
    break;
  case symbol::Binding::kLocal:
    if (entry->state == DefinitionState::kUndefined)
      entry->state = DefinitionState::kSingle;
    else if (entry->state == DefinitionState::kSingle)
      entry->state = DefinitionState::kMultiple;
    break;
  }
  return entry;
}

void symbol::Table::mark_global(const QString &name) {
  auto symbol = reference(name);
  symbol->binding = symbol::Binding::kGlobal;

  // Check for the presence of other symbols with the same name
  auto as_ptr = this->sharedFromThis();
  // Must gather all symbols from entire tree, otherwise some local references
  // may not be updated.
  auto same_name =
      symbol::selectByName(as_ptr, name, TraversalPolicy::kWholeTree);

  for (auto other : same_name) {
    if (&other->parent == &*this)
      continue; // We will be examining our symbols later.
    if (other->binding == symbol::Binding::kGlobal) {
      //  If other symbols exist with the same name in global space, mark with
      //  external multiple
      other->state = symbol::DefinitionState::kExternalMultiple;
      symbol->state = symbol::DefinitionState::kExternalMultiple;
    } else if (other->binding == symbol::Binding::kLocal) {
      other->value = QSharedPointer<symbol::value::Pointer>::create(symbol);
      // Mark the symbol as imported, so that we can tell the difference between
      // our global symbols and others' globals.
      other->binding = symbol::Binding::kImported;
      // Follow the other symbols definition state to prevent the other copy
      // from being undefined.
      other->state = symbol->state;
    }
  }
}

bool symbol::Table::exists(const QString &name) const {
  return _name_to_entry.find(name) != _name_to_entry.cend();
}
auto symbol::Table::entries() const -> symbol::Table::const_range {
  return detail::asConstKeyValueRange(this->_name_to_entry);
}

symbol::Table::range symbol::Table::entries() {
  return _name_to_entry.asKeyValueRange();
  ;
}
