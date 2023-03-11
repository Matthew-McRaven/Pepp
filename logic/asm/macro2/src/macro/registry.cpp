#include "./registry.hpp"
#include "./macro.hpp"
#include "./registered.hpp"
macro::Registry::Registry(QObject *parent) : QObject{parent} {}

bool macro::Registry::contains(QString name) const {
  return _macros.constFind(name) != _macros.constEnd();
}

const macro::Registered *macro::Registry::findMacro(QString name) const {
  if (auto macro = _macros.constFind(name); macro != _macros.constEnd())
    return macro->data();
  else
    return nullptr;
}

QList<const macro::Registered *>
macro::Registry::findMacrosByType(types::Type type) const {
  QList<const Registered *> ret;
  for (const auto &macro : _macros) {
    if (macro->type() == type)
      ret.push_back(&(*macro));
  }
  return ret;
}

void macro::Registry::clear() {
  this->_macros.clear();
  emit cleared();
}

QSharedPointer<const macro::Registered>
macro::Registry::registerMacro(types::Type type, QSharedPointer<Parsed> macro) {
  if (contains(macro->name())) {
    return nullptr;
  }
  auto registered = QSharedPointer<Registered>::create(type, macro);
  _macros[macro->name()] = registered;
  emit macrosChanged();
  return registered;
}
