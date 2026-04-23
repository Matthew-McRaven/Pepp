#include "core/compile/macro/macro_registry.hpp"

pepp::tc::MacroRegistry::MacroRegistry(std::shared_ptr<const MacroRegistry> parent) : _parent(parent), _macros() {}

std::shared_ptr<const pepp::tc::MacroDefinition> pepp::tc::MacroRegistry::find(const std::string &name,
                                                                               SearchMode mode) const {
  if (mode == SearchMode::Local || mode == SearchMode::LocalThenParent) {
    if (auto it = _macros.find(name); it != _macros.cend()) return !it->deleted ? it->definition : nullptr;
  }
  if (mode == SearchMode::Parent || mode == SearchMode::LocalThenParent)
    return _parent ? _parent->find(name, SearchMode::LocalThenParent) : nullptr;
  return nullptr;
}

bool pepp::tc::MacroRegistry::contains(const std::string &name, SearchMode mode) const {
  return find(name, mode) != nullptr;
}

void pepp::tc::MacroRegistry::purge(const std::string &name) {
  if (auto it = _macros.find(name); it != _macros.end()) {
    it->deleted = true;
  } else {
    MacroRecord record;
    record.deleted = true;
    record.definition = std::make_shared<MacroDefinition>(MacroDefinition{name, {}});
    _macros.insert(record);
  }
}

bool pepp::tc::MacroRegistry::insert(std::shared_ptr<MacroDefinition> definition, SearchMode mode) {
  if (definition == nullptr) return false;
  else if (contains(definition->name, mode)) return false;
  MacroRecord record{.deleted = false, .definition = definition};
  // Erase macro if it already exists to avoid having multiple entries with the same name.
  // Must chek that f!=end, else we might erase an invalid iterator which will cause a crash.
  if (auto f = _macros.find(definition->name); f != _macros.end()) _macros.erase(_macros.find(definition->name));
  _macros.insert(MacroRecord{.deleted = false, .definition = definition});
  return true;
}
