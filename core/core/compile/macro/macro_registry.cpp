#include "core/compile/macro/macro_registry.hpp"
#include <algorithm>

bool pepp::tc::MacroDefinition::accepts(std::size_t count) const {
  if (count > arguments.size()) return false;
  return std::all_of(arguments.begin() + count, arguments.end(),
                     [](const Argument &argument) { return argument.default_value.has_value(); });
}

pepp::tc::MacroRegistry::MacroRegistry(std::shared_ptr<const MacroRegistry> parent) : _parent(parent), _macros() {}

std::shared_ptr<const pepp::tc::MacroDefinition>
pepp::tc::MacroRegistry::find_if(const std::string &name, SearchMode mode,
                                 const std::function<bool(const MacroDefinition &)> &predicate) const {
  if (mode == SearchMode::Local || mode == SearchMode::LocalThenParent) {
    // A purge marker stands for every argument count, so it hides the parents' definitions of name. Definitions inserted
    // here after the purge still match, whatever the order of the records.
    bool purged = false;
    for (auto [it, end] = _macros.equal_range(name); it != end; ++it) {
      if (it->deleted) purged = true;
      else if (predicate(*it->definition)) return it->definition;
    }
    if (purged) return nullptr;
  }
  if (mode == SearchMode::Parent || mode == SearchMode::LocalThenParent)
    return _parent ? _parent->find_if(name, SearchMode::LocalThenParent, predicate) : nullptr;
  return nullptr;
}

std::shared_ptr<const pepp::tc::MacroDefinition> pepp::tc::MacroRegistry::find(const std::string &name,
                                                                               SearchMode mode) const {
  return find_if(name, mode, [](const MacroDefinition &) { return true; });
}

std::shared_ptr<const pepp::tc::MacroDefinition> pepp::tc::MacroRegistry::find(const std::string &name,
                                                                               std::size_t count,
                                                                               SearchMode mode) const {
  if (auto exact = find_if(name, mode, [&](const MacroDefinition &md) { return md.arguments.size() == count; }))
    return exact;
  return find_if(name, mode, [&](const MacroDefinition &md) { return md.accepts(count); });
}

bool pepp::tc::MacroRegistry::contains(const std::string &name, SearchMode mode) const {
  return find(name, mode) != nullptr;
}

void pepp::tc::MacroRegistry::purge(const std::string &name) {
  std::erase_if(_macros, [&](const MacroRecord &record) { return record.definition->name == name; });
  const auto marker = std::make_shared<MacroDefinition>(MacroDefinition{name, {}});
  _macros.insert(MacroRecord{.deleted = true, .definition = marker});
}

bool pepp::tc::MacroRegistry::insert(std::shared_ptr<MacroDefinition> definition, SearchMode mode) {
  if (definition == nullptr) return false;
  const auto arity = definition->arguments.size();
  const auto same_arity = [&](const MacroDefinition &md) { return md.arguments.size() == arity; };
  if (find_if(definition->name, mode, same_arity)) return false;
  // Replace a local definition of the same arity. A purge marker is kept, since it still hides the parents' other
  // arities. Only a (TODO) variadic definition could replace the purge marker.
  std::erase_if(_macros, [&](const MacroRecord &record) {
    return record.definition->name == definition->name && !record.deleted && same_arity(*record.definition);
  });
  _macros.insert(MacroRecord{.deleted = false, .definition = definition});
  return true;
}
