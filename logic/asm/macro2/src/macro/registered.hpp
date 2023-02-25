#pragma once

#include <QObject>

#include "./types.hpp"
namespace macro {
class Parsed;
class Registered : public QObject {
  Q_OBJECT
  Q_PROPERTY(const Parsed *contents READ contents CONSTANT)
  Q_PROPERTY(types::Type type READ type CONSTANT)
public:
  // Takes ownership of contents and changes its parent to this
  Registered(types::Type type, Parsed *contents, QObject *parent = nullptr);
  const Parsed *contents() const;
  types::Type type() const;

private:
  const Parsed *_contents;
  types::Type _type;
};
}
