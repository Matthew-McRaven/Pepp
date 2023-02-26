#pragma once

namespace pas::ast {
class Node;
}
namespace pas::ops {
template <typename T> struct ConstOp {
  virtual T operator()(const ast::Node &node) = 0;
};

template <typename T> struct MutatingOp {
  virtual T operator()(ast::Node &node) = 0;
};
}; // namespace pas::ops
