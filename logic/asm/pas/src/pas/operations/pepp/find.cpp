#include "./find.hpp"
#include "pas/ast/generic/attr_hide.hpp"
#include "pas/operations/generic/is.hpp"
#include "pas/operations/pepp/is.hpp"

bool pas::ops::pepp::detail::findNonStructural(const ast::Node &node) {
  return !pas::ops::generic::isStructural()(node);
}

bool pas::ops::pepp::detail::findUnhiddenEnd(const ast::Node &node) {
  return pas::ops::pepp::isEnd()(node) &&
         !(node.has<ast::generic::Hide>() &&
           node.get<ast::generic::Hide>().value.source);
}
