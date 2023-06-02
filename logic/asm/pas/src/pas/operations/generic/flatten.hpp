#pragma once
#include <QtCore>
#include "pas/pas_globals.hpp"

namespace pas::ast {
class Node;
} // namespace pas::ast

namespace pas::ops::generic {
void PAS_EXPORT flattenMacros(ast::Node &node);
} // namespace pas::ops::generic
