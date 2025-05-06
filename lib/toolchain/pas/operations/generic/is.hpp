/*
 * Copyright (c) 2023 J. Stanley Warford, Matthew McRaven
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once
#include <QtCore>
#include "toolchain/pas/ast/op.hpp"

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
  QList<QString> directiveAliases = {"STRING"};
  bool operator()(const ast::Node &node);
};

struct isSkip : public pas::ops::ConstOp<bool> {
  QList<QString> directiveAliases = {"SKIP"};
  bool allowFill = false;
  bool operator()(const ast::Node &node);
};

struct isByte1 : public pas::ops::ConstOp<bool> {
  QList<QString> directiveAliases = {"BYTE1"};
  bool allowMultiple = false;
  bool operator()(const ast::Node &node);
};
struct isByte2 : public pas::ops::ConstOp<bool> {
  QList<QString> directiveAliases = {"BYTE2"};
  bool allowMultiple = false;
  bool operator()(const ast::Node &node);
};
struct isSet : public pas::ops::ConstOp<bool> {
  QList<QString> directiveAliases = {"SET"};
  bool operator()(const ast::Node &node);
};

struct isOrg : public pas::ops::ConstOp<bool> {
  QList<QString> directiveAliases = {"ORG"};
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
template <typename lhs, typename rhs> struct Or : public pas::ops::ConstOp<bool> {
  lhs op1;
  rhs op2;
  bool operator()(const ast::Node &node);
};
template <typename lhs, typename rhs> struct And : public pas::ops::ConstOp<bool> {
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

template <typename op> bool pas::ops::generic::Negate<op>::operator()(const ast::Node &node) {
  return !operation(node);
}

template <typename lhs, typename rhs> bool pas::ops::generic::Or<lhs, rhs>::operator()(const ast::Node &node) {
  return op1(node) || op2(node);
}

template <typename lhs, typename rhs> bool pas::ops::generic::And<lhs, rhs>::operator()(const ast::Node &node) {
  return op1(node) && op2(node);
}

template <typename op> void pas::ops::generic::ForAll<op>::operator()(const ast::Node &node) {
  result &= operation(node);
}

template <typename op> void pas::ops::generic::Exists<op>::operator()(const ast::Node &node) {
  result |= operation(node);
}
