#pragma once
#include "pas/ast/generic/attr_error.hpp"
#include "pas/ast/generic/attr_location.hpp"
#include "pas/ast/op.hpp"
#include <QtCore>
#include "pas/pas_globals.hpp"

namespace pas::ast {
class Node;
} // namespace pas::ast

namespace pas::driver {
struct Globals;
}

namespace symbol {
class Entry;
}

namespace pas::ops::generic {
struct PAS_EXPORT LinkGlobals : public pas::ops::MutatingOp<void> {
  QSharedPointer<pas::driver::Globals> globals;
  QSet<QString> exportDirectives = {};
  void operator()(ast::Node &node);
  void updateSymbol(QSharedPointer<symbol::Entry> symbol);
};

void PAS_EXPORT linkGlobals(ast::Node &node, QSharedPointer<pas::driver::Globals> globals,
                 QSet<QString> exportDirectives);
} // namespace pas::ops::generic
