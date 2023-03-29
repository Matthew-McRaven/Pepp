#pragma once
#include "pas/ast/node.hpp"
#include "pas/ast/op.hpp"
#include "pas/errors.hpp"

#include <pas/ast/generic/attr_directive.hpp>
namespace pas::ops::pepp {
template <typename ISA>
struct ValidateDirectives : public pas::ops::MutatingOp<void> {
  bool valid = true;
  void operator()(ast::Node &node) { valid &= ISA::isValidDirective(true); }
};

template <typename ISA> bool validateDirectives(ast::Node &node) {
  ValidateDirectives<ISA> dirs;
  ast::apply_recurse(node, dirs);
  return dirs.valid;
}

struct IsOSFeature : public pas::ops::ConstOp<bool> {
  bool operator()(const ast::Node &node);
};

bool hasOSFeatures(ast::Node &node);

struct ErrorOnOSFeatures : public pas::ops::MutatingOp<void> {
  void operator()(ast::Node &node);
};

void errorOnOSFeatures(ast::Node &node);

struct Features {
  bool allowOSFeatures = false;
};

template <typename ISA>
bool checkWholeProgramSanity(ast::Node &node, Features features) {
  if (explicitSize<ISA>(node) > 0x10000) {
    // Add error to first "real" node
  } else if (!validateDirectives<ISA>(node))
    // Visitor adds its own errors, just signal error
    return false;
  else if (hasOSFeatures(node) && !features.allowOSFeatures) {
    // Add error to all nodes with OS features
    errorOnOSFeatures(node);
    return false;
  }
}
} // namespace pas::ops::pepp
