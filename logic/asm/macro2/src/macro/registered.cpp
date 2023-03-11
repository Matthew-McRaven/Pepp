#include "registered.hpp"
#include "macro.hpp"
macro::Registered::Registered(types::Type type,
                              QSharedPointer<const Parsed> contents)
    : QObject(nullptr), _contents(contents), _type(type) {}

QSharedPointer<const macro::Parsed> macro::Registered::contents() const {
  return _contents;
}

const macro::Parsed *macro::Registered::contentsPtr() const {
  return _contents.data();
}

macro::types::Type macro::Registered::type() const { return _type; }
