#pragma once
#include "pas/ast/op.hpp"
#include <QtCore>
namespace pas::ast {
class Node;
}
namespace pas::ops::generic {
// Nodes that exist to contain other nodes
struct isStructural : public pas::ops::ConstOp<bool> {
  bool operator()(const ast::Node &node);
};

struct isBlank : public pas::ops::ConstOp<bool> {
  bool operator()(const ast::Node &node);
};
struct isComment : public pas::ops::ConstOp<bool> {
  bool operator()(const ast::Node &node);
};

struct isAlign : public pas::ops::ConstOp<bool> {
  bool operator()(const ast::Node &node);
};

struct isString : public pas::ops::ConstOp<bool> {
  QList<QString> directiveAliases = {u"STRING"_qs};
  bool operator()(const ast::Node &node);
};

struct isSkip : public pas::ops::ConstOp<bool> {
  QList<QString> directiveAliases = {u"SKIP"_qs};
  bool allowFill = false;
  bool operator()(const ast::Node &node);
};

struct isByte1 : public pas::ops::ConstOp<bool> {
  QList<QString> directiveAliases = {u"BYTE1"_qs};
  bool allowMultiple = false;
  bool operator()(const ast::Node &node);
};
struct isByte2 : public pas::ops::ConstOp<bool> {
  QList<QString> directiveAliases = {u"BYTE2"_qs};
  bool allowMultiple = false;
  bool operator()(const ast::Node &node);
};
struct isSet : public pas::ops::ConstOp<bool> {
  QList<QString> directiveAliases = {u"SET"_qs};
  bool operator()(const ast::Node &node);
};

struct isDirective : public pas::ops::ConstOp<bool> {
  bool operator()(const ast::Node &node);
};

struct isMacro : public pas::ops::ConstOp<bool> {
  bool operator()(const ast::Node &node);
};

struct SourceHidden : public pas::ops::ConstOp<bool> {
  bool operator()(const ast::Node &node);
};

struct ListingHidden : public pas::ops::ConstOp<bool> {
  bool operator()(const ast::Node &node);
};

template <typename op> struct Negate : public pas::ops::ConstOp<bool> {
  op operation;
  bool operator()(const ast::Node &node);
};
template <typename lhs, typename rhs>
struct Or : public pas::ops::ConstOp<bool> {
  lhs op1;
  rhs op2;
  bool operator()(const ast::Node &node);
};
template <typename lhs, typename rhs>
struct And : public pas::ops::ConstOp<bool> {
  lhs op1;
  rhs op2;
  bool operator()(const ast::Node &node);
};
} // namespace pas::ops::generic

template <typename op>
bool pas::ops::generic::Negate<op>::operator()(const ast::Node &node) {
  return !operation(node);
}

template <typename lhs, typename rhs>
bool pas::ops::generic::Or<lhs, rhs>::operator()(const ast::Node &node) {
  return op1(node) || op2(node);
}

template <typename lhs, typename rhs>
bool pas::ops::generic::And<lhs, rhs>::operator()(const ast::Node &node) {
  return op1(node) && op2(node);
}
