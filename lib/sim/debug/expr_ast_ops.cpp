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

void pepp::debug::detail::IsConstantExpressionVisitor::accept(const Constant &node) {}

void pepp::debug::detail::IsConstantExpressionVisitor::accept(const BinaryInfix &node) {
  switch (node._op) {
  case BinaryInfix::Operators::DOT:
  case BinaryInfix::Operators::STAR_DOT: is_constant_expression = false; return;
  default: break;
  }
  if (node._arg1->accept(*this); !is_constant_expression) return;
  node._arg2->accept(*this);
}

void pepp::debug::detail::IsConstantExpressionVisitor::accept(const UnaryPrefix &node) {
  switch (node._op) {
  case UnaryPrefix::Operators::ADDRESS_OF:
  case UnaryPrefix::Operators::DEREFERENCE: is_constant_expression = false; return;
  default: break;
  }
  node._arg->accept(*this);
}

void pepp::debug::detail::IsConstantExpressionVisitor::accept(const Parenthesized &node) { node._term->accept(*this); }

void pepp::debug::detail::GatherVolatileTerms::accept(const Variable &node) {
  volatiles.insert(node.shared_from_this());
}

void pepp::debug::detail::GatherVolatileTerms::accept(const Constant &node) {}

void pepp::debug::detail::GatherVolatileTerms::accept(const BinaryInfix &node) {
  if (node.cv_qualifiers() & CVQualifiers::Volatile) volatiles.insert(node.shared_from_this());
  node._arg1->accept(*this);
  node._arg2->accept(*this);
}

void pepp::debug::detail::GatherVolatileTerms::accept(const UnaryPrefix &node) {
  if (node.cv_qualifiers() & CVQualifiers::Volatile) volatiles.insert(node.shared_from_this());
  node._arg->accept(*this);
}

void pepp::debug::detail::GatherVolatileTerms::accept(const Parenthesized &node) { node._term->accept(*this); }
