#pragma once
#include "pas/ast/op.hpp"
#include <QtCore>
namespace pas::ast {
class Node;
namespace generic {
class Message;
class SourceLocation;
} // namespace generic
} // namespace pas::ast

namespace pas::ops::generic {
struct collectErrors : public pas::ops::ConstOp<void> {
  QList<QPair<ast::generic::SourceLocation, ast::generic::Message>> errors;
  void operator()(const ast::Node &node);
};
} // namespace pas::ops::generic
