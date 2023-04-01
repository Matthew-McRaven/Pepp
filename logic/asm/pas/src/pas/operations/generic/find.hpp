#pragma once
#include <QtCore>
#include "pas/ast/op.hpp"

namespace pas::ops::generic {

using SelectorFn = std::function<bool(const ast::Node&)>;
QSharedPointer<const pas::ast::Node> findFirst(const ast::Node& node, SelectorFn selector);
QSharedPointer<pas::ast::Node> findFirst(ast::Node& node, SelectorFn selector);
}

