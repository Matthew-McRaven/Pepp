#pragma once
#include "obj/mmio.hpp"
#include "pas/ast/node.hpp"
#include "pas/ast/op.hpp"

namespace pas::ops::pepp {
struct GatherIODefinitions : public pas::ops::ConstOp<void> {

  void operator()(const pas::ast::Node &node) override;
  QList<::obj::IO> ios;
};

QList<::obj::IO> gatherIODefinitions(const pas::ast::Node &node);
} // namespace pas::ops::pepp
