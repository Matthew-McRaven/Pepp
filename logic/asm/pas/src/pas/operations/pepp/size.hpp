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
#include "pas/ast/generic/attr_children.hpp"
#include "pas/ast/generic/attr_comment.hpp"
#include "pas/ast/generic/attr_directive.hpp"
#include "pas/ast/generic/attr_macro.hpp"
#include "pas/ast/generic/attr_symbol.hpp"
#include "pas/ast/node.hpp"
#include "pas/ast/op.hpp"
#include "pas/ast/value/base.hpp"
#include "pas/operations/generic/is.hpp"
#include "pas/operations/pepp/is.hpp"
#include <QtCore>
#include "pas/pas_globals.hpp"

namespace pas::ops::pepp {

enum class Direction { Forward, Backward };

Direction PAS_EXPORT direction(const ast::Node &node);
quint16 PAS_EXPORT baseAddress(const ast::Node &node);
// Use when you have a root node, and you don't have a pre-specified `at`.
// If you want to place your code at a custom location in memory, use explicit
// size. Will search for a BURN, and use the `at=argument`. Otherwise `at=0`.
template <typename ISA> qsizetype implicitSize(const ast::Node &node);

// Makes no assumption about if the node is a root, or any placement parameters.
// Use this if you want to place code at a non-standard location in memory.
template <typename ISA>
qsizetype explicitSize(const ast::Node &node, quint16 at,
                       Direction direction = Direction::Forward);

// Compute the size of a contiguous region from the addresses.
qsizetype PAS_EXPORT sizeFromAddress(const ast::Node &node);

namespace detail {
quint16 PAS_EXPORT sizeAlign(const ast::Node, quint16 at, Direction direction);
quint16 PAS_EXPORT sizeASCII(const ast::Node, quint16 at, Direction direction);
quint16 PAS_EXPORT sizeBlock(const ast::Node, quint16 at, Direction direction);
quint16 PAS_EXPORT sizeByte(const ast::Node, quint16 at, Direction direction);
quint16 PAS_EXPORT sizeWord(const ast::Node, quint16 at, Direction direction);
} // namespace detail
} // namespace pas::ops::pepp

template <typename ISA>
qsizetype pas::ops::pepp::implicitSize(const pas::ast::Node &node) {
  return explicitSize<ISA>(node, baseAddress(node), direction(node));
}
template <typename ISA>
qsizetype pas::ops::pepp::explicitSize(const ast::Node &node, quint16 at,
                                       Direction direction) {
  using sizeFn = std::function<quint16(const ast::Node &, quint16, Direction)>;
  static const QMap<QString, sizeFn> directiveMap = {
      {u"ALIGN"_qs, &detail::sizeAlign},
      {u"ASCII"_qs, &detail::sizeASCII},
      {u"BLOCK"_qs, &detail::sizeBlock},
      {u"BYTE"_qs, &detail::sizeByte},
      {u"WORD"_qs, &detail::sizeWord}};
  if (generic::isDirective()(node)) {
    auto name = node.get<ast::generic::Directive>().value;
    if (auto item = directiveMap.find(name.toUpper());
        item != directiveMap.end()) {
      return item.value()(node, at, direction);
    }
    return 0;
  } else if (generic::isMacro()(node) || generic::isStructural()(node)) {
    qsizetype ret = 0;
    for (auto &child : node.get<ast::generic::Children>().value) {
      auto innerAt = at + (direction == Direction::Forward ? ret : -ret);
      ret += explicitSize<ISA>(*child, innerAt, direction);
    }
    return ret;
  } else if (pepp::isUnary<ISA>()(node))
    return 1;
  else if (pepp::isNonUnary<ISA>()(node))
    return 3;
  else
    return 0;
}
