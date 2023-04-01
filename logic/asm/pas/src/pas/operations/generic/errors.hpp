#pragma once
#include "pas/ast/generic/attr_error.hpp"
#include "pas/ast/generic/attr_location.hpp"
#include "pas/ast/op.hpp"
#include <QtCore>
namespace pas::ast {
class Node;
} // namespace pas::ast

namespace pas::ops::generic {
struct CollectErrors : public pas::ops::ConstOp<void> {
  QList<QPair<ast::generic::SourceLocation, ast::generic::Message>> errors;
  void operator()(const ast::Node &node);
};

QList<QPair<ast::generic::SourceLocation, ast::generic::Message>> collectErrors(const ast::Node& node);
} // namespace pas::ops::generic
