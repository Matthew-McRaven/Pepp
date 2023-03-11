#pragma once
#include "pas/ast/generic/attr_error.hpp"
#include "pas/ast/generic/attr_location.hpp"
#include "pas/ast/op.hpp"
#include <QtCore>
namespace pas::ast {
class Node;
} // namespace pas::ast

namespace pas::driver {
class Globals;
}

namespace symbol {
class Entry;
}

namespace pas::ops::generic {
struct LinkGlobals : public pas::ops::MutatingOp<void> {
  QSharedPointer<pas::driver::Globals> globals;
  QSet<QString> exportDirectives = {};
  void operator()(ast::Node &node);
  void updateSymbol(QSharedPointer<symbol::Entry> symbol);
};
} // namespace pas::ops::generic
