#pragma once
#include "sim/debug/expr_ast.hpp"
namespace pepp::debug {
bool is_constant_expression(const Term &term);
std::vector<std::shared_ptr<const Term>> volatiles(const Term &term);
namespace detail {
struct IsConstantExpressionVisitor : public ConstantTermVisitor {
  bool is_constant_expression = true;
  void accept(const Variable &node) override;
  void accept(const Constant &node) override;
  void accept(const BinaryInfix &node) override;
  void accept(const UnaryPrefix &node) override;
  void accept(const Parenthesized &node) override;
};
struct GatherVolatileTerms : public ConstantTermVisitor {
  // Use set to de-duplicate repeated terms.
  std::set<std::shared_ptr<const Term>> volatiles;
  void accept(const Variable &node) override;
  void accept(const Constant &node) override;
  void accept(const BinaryInfix &node) override;
  void accept(const UnaryPrefix &node) override;
  void accept(const Parenthesized &node) override;
};
} // namespace detail

} // namespace pepp::debug
