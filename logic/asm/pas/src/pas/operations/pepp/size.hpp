#pragma once
#include "./string.hpp"
#include "pas/ast/generic/attr_macro.hpp"
#include "pas/ast/node.hpp"
#include "pas/ast/op.hpp"
#include "pas/ast/value/base.hpp"
#include <QtCore>
#include <pas/ast/generic/attr_children.hpp>
#include <pas/ast/generic/attr_comment.hpp>
#include <pas/ast/generic/attr_symbol.hpp>

namespace pas::ops::pepp {

enum class Direction { Forward, Backward };

template <typename ISA>
quint16 size(const ast::Node &node, quint16 at,
             Direction direction = Direction::Forward);

namespace detail {
quint16 sizeAlign(const ast::Node, quint16 at, Direction direction);
quint16 sizeASCII(const ast::Node, quint16 at, Direction direction);
quint16 sizeBlock(const ast::Node, quint16 at, Direction direction);
quint16 sizeByte(const ast::Node, quint16 at, Direction direction);
quint16 sizeWord(const ast::Node, quint16 at, Direction direction);
} // namespace detail
} // namespace pas::ops::pepp

template <typename ISA>
quint16 pas::ops::pepp::size(const ast::Node &node, quint16 at,
                             Direction direction) {
  using sizeFn = std::function<quint16(const ast::Node &, quint16, Direction)>;
  static const QMap<QString, sizeFn> directiveMap = {
      {u"ALIGN"_qs, &detail::sizeAlign},
      {u"ASCII"_qs, &detail::sizeASCII},
      {u"BLOCK"_qs, &detail::sizeBlock},
      {u"BYTE"_qs, &detail::sizeByte},
      {u"WORD"_qs, &detail::sizeWord}};
  if (generic::isDirective()(node)) {
    auto macroName = node.get<ast::generic::Macro>().value;
    if (auto item = directiveMap.find(macroName.toUpper());
        item != directiveMap.end()) {
      return item.value()(node, at, direction);
    }
    return 0;
  } else if (generic::isMacro()(node)) {
    quint16 ret = 0;
    for (auto &child : node.get<ast::generic::Children>().value) {
      auto innerAt = at + (direction == Direction::Forward ? ret : -ret);
      ret += size<ISA>(*child, innerAt, direction);
    }
    return ret;
  } else if (pepp::isUnary<ISA>()(node))
    return 1;
  else if (pepp::isNonUnary<ISA>()(node))
    return 3;
  else
    return 0;
}
