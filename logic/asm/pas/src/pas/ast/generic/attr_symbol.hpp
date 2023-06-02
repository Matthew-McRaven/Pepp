#pragma once
#include <QtCore>
#include "pas/pas_globals.hpp"

namespace symbol {
class Entry;
class Table;
} // namespace symbol

namespace pas::ast::generic {
struct PAS_EXPORT SymbolDeclaration {
  static const inline QString attributeName = u"generic:symbol_decl"_qs;
  QSharedPointer<symbol::Entry> value = {};
  bool operator==(const SymbolDeclaration &other) const = default;
};

struct PAS_EXPORT SymbolTable {
  static const inline QString attributeName = u"generic:symbol_table"_qs;
  QSharedPointer<symbol::Table> value = {};
  bool operator==(const SymbolTable &other) const = default;
};

} // namespace pas::ast::generic
Q_DECLARE_METATYPE(pas::ast::generic::SymbolDeclaration);
Q_DECLARE_METATYPE(pas::ast::generic::SymbolTable);
