#pragma once
#include "pas/operations/generic/find.hpp"

namespace pas::ops::pepp {
namespace detail {
bool findNonStructural(const ast::Node &node);
bool findUnhiddenEnd(const ast::Node &node);
} // namespace detail
static const ops::generic::SelectorFn findNonStructural =
    detail::findNonStructural;
static const ops::generic::SelectorFn findUnhiddenEnd = detail::findUnhiddenEnd;
} // namespace pas::ops::pepp
