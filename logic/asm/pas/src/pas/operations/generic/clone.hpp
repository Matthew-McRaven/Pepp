#pragma once
#include "pas/ast/generic/attr_sec.hpp"
#include "pas/ast/op.hpp"
#include <QtCore>
#include "pas/pas_globals.hpp"

namespace pas::ast {
class Node;
}
namespace symbol {
class Entry;
class Table;
class ForkMap;
} // namespace symbol

namespace pas::ops::generic {
struct PAS_EXPORT clone : public ConstOp<QSharedPointer<ast::Node>> {
  QSharedPointer<symbol::ForkMap> mapping;
  QSharedPointer<ast::Node> operator()(const ast::Node &node) override;

private:
  QSharedPointer<symbol::Entry> entry(const symbol::Entry *entry);
  QSharedPointer<symbol::Table> table(const symbol::Table *table);
};
} // namespace pas::ops::generic
