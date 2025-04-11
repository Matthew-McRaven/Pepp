#include "expr_ast_ops.hpp"

bool pepp::debug::is_constant_expression(const Term &term) {
  detail::IsConstantExpressionVisitor v;
  term.accept(v);
  return v.is_constant_expression;
}
std::vector<std::shared_ptr<const pepp::debug::Term>> pepp::debug::volatiles(const Term &term) {
  detail::GatherVolatileTerms v;
  term.accept(v);
  auto ret = std::vector<std::shared_ptr<const pepp::debug::Term>>(v.volatiles.cbegin(), v.volatiles.cend());
  return ret;
}

void pepp::debug::detail::IsConstantExpressionVisitor::accept(const Variable &node) { is_constant_expression = false; }

void pepp::debug::detail::IsConstantExpressionVisitor::accept(const DebuggerVariable &node) {
  is_constant_expression = false;
}

void pepp::debug::detail::IsConstantExpressionVisitor::accept(const Constant &node) {}

void pepp::debug::detail::IsConstantExpressionVisitor::accept(const BinaryInfix &node) {
  switch (node.op) {
  case BinaryInfix::Operators::DOT:
  case BinaryInfix::Operators::STAR_DOT: is_constant_expression = false; return;
  default: break;
  }
  if (node.lhs->accept(*this); !is_constant_expression) return;
  node.rhs->accept(*this);
}

void pepp::debug::detail::IsConstantExpressionVisitor::accept(const UnaryPrefix &node) {
  switch (node.op) {
  case UnaryPrefix::Operators::ADDRESS_OF:
  case UnaryPrefix::Operators::DEREFERENCE: is_constant_expression = false; return;
  default: break;
  }
  node.arg->accept(*this);
}

void pepp::debug::detail::IsConstantExpressionVisitor::accept(const Parenthesized &node) { node.term->accept(*this); }

void pepp::debug::detail::IsConstantExpressionVisitor::accept(const ExplicitCast &node) { node.arg->accept(*this); }

void pepp::debug::detail::GatherVolatileTerms::accept(const Variable &node) {
  volatiles.insert(node.shared_from_this());
}

void pepp::debug::detail::GatherVolatileTerms::accept(const DebuggerVariable &node) {
  volatiles.insert(node.shared_from_this());
}

void pepp::debug::detail::GatherVolatileTerms::accept(const Constant &node) {}

void pepp::debug::detail::GatherVolatileTerms::accept(const BinaryInfix &node) {
  if (node.cv_qualifiers() & CVQualifiers::Volatile) volatiles.insert(node.shared_from_this());
  node.lhs->accept(*this);
  node.rhs->accept(*this);
}

void pepp::debug::detail::GatherVolatileTerms::accept(const UnaryPrefix &node) {
  if (node.cv_qualifiers() & CVQualifiers::Volatile) volatiles.insert(node.shared_from_this());
  node.arg->accept(*this);
}

void pepp::debug::detail::GatherVolatileTerms::accept(const Parenthesized &node) { node.term->accept(*this); }

void pepp::debug::detail::GatherVolatileTerms::accept(const ExplicitCast &node) { node.arg->accept(*this); }

void pepp::debug::mark_parents_dirty(Term &base) {
  if (base.dirty()) return;
  for (const auto &weak : base.dependents()) {
    if (weak.expired()) continue;
    auto shared = weak.lock();
    mark_parents_dirty(*shared);
  }
  base.mark_dirty();
}
