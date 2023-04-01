#pragma once
#include <QtCore>
#include "pas/ast/op.hpp"

namespace pas::ops::pepp {

typedef bool(*SelectorFn)(const ast::Node&);
QSharedPointer<const pas::ast::Node> findFirst(const ast::Node& node, SelectorFn selector);
QSharedPointer<pas::ast::Node> findFirst(ast::Node& node, SelectorFn selector);
}

