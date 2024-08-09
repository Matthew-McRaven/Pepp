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
#include "asm/pas/ast/op.hpp"
#include "asm/pas/pas_globals.hpp"

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
