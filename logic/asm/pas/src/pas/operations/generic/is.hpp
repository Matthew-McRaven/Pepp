#pragma once
#include "pas/ast/op.hpp"
#include <QtCore>
#include "pas/pas_globals.hpp"

namespace pas::ast {
class Node;
}
namespace pas::ops::generic {
// Nodes that exist to contain other nodes
struct PAS_EXPORT isStructural : public pas::ops::ConstOp<bool> {
  bool operator()(const ast::Node &node);
};

struct PAS_EXPORT isBlank : public pas::ops::ConstOp<bool> {
  bool operator()(const ast::Node &node);
};
struct PAS_EXPORT isComment : public pas::ops::ConstOp<bool> {
  bool operator()(const ast::Node &node);
};

struct PAS_EXPORT isAlign : public pas::ops::ConstOp<bool> {
  bool operator()(const ast::Node &node);
};

struct PAS_EXPORT isString : public pas::ops::ConstOp<bool> {
  QList<QString> directiveAliases = {u"STRING"_qs};
  bool operator()(const ast::Node &node);
};

struct PAS_EXPORT isSkip : public pas::ops::ConstOp<bool> {
  QList<QString> directiveAliases = {u"SKIP"_qs};
  bool allowFill = false;
  bool operator()(const ast::Node &node);
};

struct PAS_EXPORT isByte1 : public pas::ops::ConstOp<bool> {
  QList<QString> directiveAliases = {u"BYTE1"_qs};
  bool allowMultiple = false;
  bool operator()(const ast::Node &node);
};
struct PAS_EXPORT isByte2 : public pas::ops::ConstOp<bool> {
  QList<QString> directiveAliases = {u"BYTE2"_qs};
  bool allowMultiple = false;
  bool operator()(const ast::Node &node);
};
struct PAS_EXPORT isSet : public pas::ops::ConstOp<bool> {
  QList<QString> directiveAliases = {u"SET"_qs};
  bool operator()(const ast::Node &node);
};

struct PAS_EXPORT isOrg : public pas::ops::ConstOp<bool> {
  QList<QString> directiveAliases = {u"ORG"_qs};
  bool operator()(const ast::Node &node);
};

struct PAS_EXPORT isDirective : public pas::ops::ConstOp<bool> {
  bool operator()(const ast::Node &node);
};

struct PAS_EXPORT isMacro : public pas::ops::ConstOp<bool> {
  bool operator()(const ast::Node &node);
};

struct PAS_EXPORT SourceHidden : public pas::ops::ConstOp<bool> {
  bool operator()(const ast::Node &node);
};

struct PAS_EXPORT ListingHidden : public pas::ops::ConstOp<bool> {
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

template <typename op> struct ForAll : public pas::ops::ConstOp<void> {
  bool result = true;
  op operation;
  void operator()(const ast::Node &node);
};

template <typename op> struct Exists : public pas::ops::ConstOp<void> {
  bool result = false;
  op operation;
  void operator()(const ast::Node &node);
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

template <typename op>
void pas::ops::generic::ForAll<op>::operator()(const ast::Node &node) {
  result &= operation(node);
}

template <typename op>
void pas::ops::generic::Exists<op>::operator()(const ast::Node &node) {
  result |= operation(node);
}
