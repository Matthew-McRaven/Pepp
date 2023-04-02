#pragma once
#include "pas/operations/generic/find.hpp"

namespace pas::ops::pepp {
namespace detail {
bool findNonStructural(const ast::Node &node);
}
static const ops::generic::SelectorFn findNonStructural =
    detail::findNonStructural;
} // namespace pas::ops::pepp
