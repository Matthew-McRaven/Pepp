#pragma once
#pragma once
#include "pas/ast/op.hpp"
#include "pas/operations/pepp/size.hpp"
#include <QtCore>

namespace pas::ast {
class Node;
} // namespace pas::ast

namespace pas::ops::pepp {

namespace detail {
// TODO: extend to handle sections
template <typename ISA>
void assignAddressesImpl(ast::Node &node, quint16 &start,
                         Direction direction = Direction::Forward);
bool hasBurn(QList<QSharedPointer<ast::Node>> &list);
quint16 getBurnArg(QList<QSharedPointer<ast::Node>> &list);
} // namespace detail
// Only works if the given a single section.
template <typename ISA> void assignAddresses(ast::Node &section);
} // namespace pas::ops::pepp

template <typename ISA>
void pas::ops::pepp::detail::assignAddressesImpl(ast::Node &node, quint16 &base,
                                                 Direction direction) {
  auto size = pepp::size<ISA>(node, base, direction);
  auto symBase = base;
  quint16 newBase = base;
  if (direction == Direction::Forward) {
    // Must explicitly handle address wrap-around, because math inside set
    // address widens implicitly.
    newBase = (base + size) % 0xFFFF;
    // size is 1-index, while base is 0-indexed. Offset by 1. Unless size is 0,
    // in which case no adjustment is necessary.
    ast::setAddress(node, base, (newBase - (size > 0 ? 1 : 0)) % 0xFFFF);
    base = newBase;
  } else {
    newBase = (base - size) % 0xFFFF;
    // size is 1-index, while base is 0-indexed. Offset by 1. Unless size is 0,
    // in which case no adjustment is necessary.
    ast::setAddress(node, (newBase + (size > 0 ? 1 : 0)) % 0xFFFF, base);
    base = newBase;
    symBase = base;
  }
  if (node.has<ast::generic::SymbolDeclaration>()) {
    auto isCode = node.has<ast::pepp::Instruction<ISA>>();
    auto symbol = node.get<ast::generic::SymbolDeclaration>().value;
    symbol->value = QSharedPointer<symbol::value::Location>::create(
        2, symBase, 0, isCode ? symbol::Type::kCode : symbol::Type::kObject);
  }
}

template <typename ISA> void pas::ops::pepp::assignAddresses(ast::Node &root) {
  quint16 base = 0;
  if (root.has<ast::generic::Children>()) {
    auto children = root.get<ast::generic::Children>().value;
    if (detail::hasBurn(children)) {
      base = detail::getBurnArg(children);
      for (auto child = children.rbegin(); child != children.rend(); ++child)
        detail::assignAddressesImpl<ISA>(**child, base, Direction::Backward);
    } else {
      for (auto &child : children)
        detail::assignAddressesImpl<ISA>(*child, base, Direction::Forward);
    }
  }
}
