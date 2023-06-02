#pragma once
#include "pas/ast/node.hpp"
#include "pas/ast/op.hpp"
#include <QtCore>
#include "pas/pas_globals.hpp"

namespace pas::ops::generic {
struct PAS_EXPORT SuppressObject : public pas::ops::MutatingOp<void> {
  void operator()(ast::Node &node) override;
};
} // namespace pas::ops::generic
