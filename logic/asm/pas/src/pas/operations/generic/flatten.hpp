#pragma once
#include <QtCore>
namespace pas::ast {
class Node;
} // namespace pas::ast

namespace pas::ops::generic {
void flattenMacros(ast::Node &node);
} // namespace pas::ops::generic
