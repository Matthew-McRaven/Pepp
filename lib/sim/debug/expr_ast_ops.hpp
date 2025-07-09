#pragma once
#include <set>
#include "sim/debug/expr_ast.hpp"

namespace pepp::debug {
bool is_constant_expression(const Term &term);
std::vector<std::shared_ptr<Term>> volatiles(Term &term);
void mark_parents_dirty(Term &base);

namespace detail {
struct IsConstantExpressionVisitor : public ConstantTermVisitor {
  bool is_constant_expression = true;
  void accept(const Variable &node) override;
  void accept(const DebuggerVariable &node) override;
  void accept(const Constant &node) override;
  void accept(const BinaryInfix &node) override;
  void accept(const UnaryPrefix &node) override;
  void accept(const MemoryRead &node) override;
  void accept(const Parenthesized &node) override;
  void accept(const DirectCast &node) override;
  void accept(const IndirectCast &node) override;
};

// Mutating because used may want to evaluate() on gathered terms, which is non-const
struct GatherVolatileTerms : public MutatingTermVisitor {
  // Use set to de-duplicate repeated terms.
  std::set<std::shared_ptr<Term>> volatiles;
  std::vector<std::shared_ptr<Term>> to_vector();
  void accept(Variable &node) override;
  void accept(DebuggerVariable &node) override;
  void accept(Constant &node) override;
  void accept(BinaryInfix &node) override;
  void accept(UnaryPrefix &node) override;
  void accept(MemoryRead &node) override;
  void accept(Parenthesized &node) override;
  void accept(DirectCast &node) override;
  void accept(IndirectCast &node) override;
};
} // namespace detail

} // namespace pepp::debug
