#pragma once
#include "pas/ast/node.hpp"
#include "pas/ast/op.hpp"
namespace pas::ops::pepp {

struct IO {
  QString name;
  enum class Direction { kInput, kOutput } direction;
};

struct GatherIODefinitions : public pas::ops::ConstOp<void> {

  void operator()(const pas::ast::Node &node) override;
  QList<IO> ios;
};

QList<IO> gatherIODefinitions(const pas::ast::Node &node);
} // namespace pas::ops::pepp
