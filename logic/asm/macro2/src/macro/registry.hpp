#pragma once

#include <QObject>

#include "./types.hpp"
#include "macro_globals.hpp"

namespace macro {

class Parsed;
class Registered;

class MACRO_EXPORT Registry : public QObject {
  Q_OBJECT
public:
  explicit Registry(QObject *parent = nullptr);
  bool contains(QString name) const;
  // Returns nullptr if not found.
  const Registered *findMacro(QString name) const;
  // FIXME: Replace with an iterator so as not to force additional memory
  // allocations.
  QList<const Registered *> findMacrosByType(types::Type type) const;
  void clear();
  // Ownership of macro is always transfered to this.
  // Returns nullptr if the macro already exists in the registry. In this case,
  // registry will delete macro.
  // Returned pointer is non-owning
  QSharedPointer<const Registered> registerMacro(types::Type type,
                                                 QSharedPointer<Parsed> macro);

signals:
  //! Emitted when a macro is successfully registered.
  void macrosChanged();
  //! Emitted when clear() is called.
  void cleared();

private:
  //
  QMap<QString, QSharedPointer<Registered>> _macros;
};
} // namespace macro
