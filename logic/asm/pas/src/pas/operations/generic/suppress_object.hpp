#pragma once
#include "pas/ast/node.hpp"
#include "pas/ast/op.hpp"
#include <QtCore>

namespace pas::ops::generic {
struct SuppressObject : public pas::ops::MutatingOp<void> {
  void operator()(ast::Node &node) override;
};
} // namespace pas::ops::generic
