#pragma once

#include <QObject>

#include "./types.hpp"
#include "macro_globals.hpp"

namespace macro {
class Parsed;
class MACRO_EXPORT Registered : public QObject {
  Q_OBJECT
  Q_PROPERTY(const Parsed *contents READ contentsPtr CONSTANT)
  Q_PROPERTY(types::Type type READ type CONSTANT)
public:
  // Takes ownership of contents and changes its parent to this
  Registered(types::Type type, QSharedPointer<const Parsed> contents);
  // Needed to access from QML.
  const Parsed *contentsPtr() const;
  QSharedPointer<const Parsed> contents() const;
  types::Type type() const;

private:
  QSharedPointer<const Parsed> _contents;
  types::Type _type;
};
} // namespace macro
