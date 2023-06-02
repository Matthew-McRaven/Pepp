#pragma once
#include "pas/operations/generic/find.hpp"
#include "pas/pas_globals.hpp"

namespace pas::ops::pepp {
namespace detail {
bool PAS_EXPORT findNonStructural(const ast::Node &node);
bool PAS_EXPORT findUnhiddenEnd(const ast::Node &node);
} // namespace detail
inline const ops::generic::SelectorFn findNonStructural =
    detail::findNonStructural;
inline const ops::generic::SelectorFn findUnhiddenEnd = detail::findUnhiddenEnd;
} // namespace pas::ops::pepp
