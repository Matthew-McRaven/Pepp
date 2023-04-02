#include "./find.hpp"
#include "pas/operations/generic/is.hpp"

bool pas::ops::pepp::detail::findNonStructural(const ast::Node &node) {
  return !pas::ops::generic::isStructural()(node);
}
