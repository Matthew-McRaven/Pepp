#pragma once
#include <QtCore>

#include "symbol_globals.hpp"

namespace symbol {
class Table;
class Entry;
class SYMBOL_EXPORT ForkMap {
public:
  void addMapping(const Table *from, QSharedPointer<Table> to);
  void addMapping(const Entry *from, QSharedPointer<Entry> to);
  QSharedPointer<Table> map(const Table *from);
  QSharedPointer<Entry> map(const Entry *from);

private:
  QMap<const Table *, QSharedPointer<Table>> _tableMap;
  QMap<const Entry *, QSharedPointer<Entry>> _symbolMap;
};

QSharedPointer<ForkMap> SYMBOL_EXPORT fork(QSharedPointer<const symbol::Table> from);
} // namespace symbol
