#pragma once
#include <QtCore>
#include "pas/ast/op.hpp"
#include "pas/pas_globals.hpp"

namespace pas::ops::generic {

using SelectorFn = std::function<bool(const ast::Node&)>;
QSharedPointer<const pas::ast::Node> PAS_EXPORT findFirst(const ast::Node& node, SelectorFn selector);
QSharedPointer<pas::ast::Node> PAS_EXPORT findFirst(ast::Node& node, SelectorFn selector);
}

