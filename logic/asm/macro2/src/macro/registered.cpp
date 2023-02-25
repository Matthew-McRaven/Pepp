#include "registered.hpp"
#include "macro.hpp"
macro::Registered::Registered(types::Type type, Parsed *contents,
                              QObject *parent)
    : QObject(parent), _contents(contents), _type(type) {
  contents->setParent(this);
}

const macro::Parsed *macro::Registered::contents() const { return _contents; }

macro::types::Type macro::Registered::type() const { return _type; }
