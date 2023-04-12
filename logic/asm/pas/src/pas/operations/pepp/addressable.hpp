#pragma once

#include "pas/ast/generic/attr_directive.hpp"
#include "pas/ast/node.hpp"
#include "pas/operations/generic/is.hpp"
#include "pas/operations/pepp/is.hpp"

namespace pas::ops::pepp {
template <typename ISA> bool isAddressable(const ast::Node &node);
}

template <typename ISA>
bool pas::ops::pepp::isAddressable(const ast::Node &node) {
  static const auto addressableDirectives =
      QSet<QString>{"ALIGN", "ASCII", "BLOCK", "BYTE", "WORD"};
  if (generic::isDirective()(node) &&
      addressableDirectives.contains(
          node.get<ast::generic::Directive>().value.toUpper()))
    return true;
  return isUnary<ISA>()(node) || isNonUnary<ISA>()(node);
}
