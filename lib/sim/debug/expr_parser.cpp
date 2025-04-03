#include "expr_parser.hpp"

#include "expr_ast.hpp"

pepp::debug::Parser::Parser(ExpressionCache &cache) : _cache(cache) {}

std::shared_ptr<pepp::debug::Variable> pepp::debug::Parser::parse_identifier(TokenBuffer &tok) {
  using ID = detail::Identifier;
  if (auto maybe_ident = tok.match<ID>(); !std::holds_alternative<ID>(maybe_ident)) {
    return nullptr;
  } else {
    return accept(Variable(std::get<ID>(maybe_ident)));
  }
}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_value(TokenBuffer &tok) {
  if (auto maybe_reg = parse_register(tok); maybe_reg != nullptr) return maybe_reg;
  else if (auto maybe_constant = parse_constant(tok); maybe_constant != nullptr) return maybe_constant;
  else if (auto maybe_ident = parse_identifier(tok); maybe_ident != nullptr) return maybe_ident;
  return nullptr;
}

std::shared_ptr<pepp::debug::Register> pepp::debug::Parser::parse_register(TokenBuffer &tok) { return nullptr; }

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_expression(TokenBuffer &tok) { return nullptr; }

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_member_access(TokenBuffer &tok) {

  using LIT = detail::Literal;
  using ID = detail::Identifier;
  using Ops = BinaryInfix::Operators;
  auto cp = TokenCheckpoint(tok);
  auto maybe_ident = parse_identifier(tok);

  if (maybe_ident == nullptr) return cp.rollback<pepp::debug::Term>();

  auto maybe_lit = tok.match<LIT>();
  if (!std::holds_alternative<LIT>(maybe_lit)) return cp.rollback<pepp::debug::Term>();

  LIT lit = std::get<LIT>(maybe_lit);
  if (auto lit = std::get<LIT>(maybe_lit); lit.literal != "." && lit.literal != "->")
    return cp.rollback<pepp::debug::Term>();

  auto op = lit.literal == "." ? Ops::DOT : Ops::STAR_DOT;
  auto maybe_ident2 = parse_identifier(tok);
  if (maybe_ident2 == nullptr) cp.rollback<pepp::debug::Term>();

  return accept(BinaryInfix(op, maybe_ident, maybe_ident2));
}
std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_p0(TokenBuffer &tok) {
  auto cp = TokenCheckpoint(tok);
  if (auto maybe_member_access = parse_member_access(tok); maybe_member_access != nullptr) return maybe_member_access;
  else if (auto value = parse_value(tok); value != nullptr) return value;
  return cp.rollback<pepp::debug::Term>();
}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_p1(TokenBuffer &tok) { return parse_p0(tok); }

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_p2(TokenBuffer &tok) { return parse_p1(tok); }

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_p3(TokenBuffer &tok) { return parse_p2(tok); }

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_p4(TokenBuffer &tok) { return parse_p3(tok); }

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_p5(TokenBuffer &tok) { return parse_p4(tok); }

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_parened(TokenBuffer &tok) {
  using Lit = detail::Literal;
  auto cp = TokenCheckpoint(tok);
  if (auto maybe_open = tok.match<Lit>(); !std::holds_alternative<Lit>(maybe_open)) {
    return cp.rollback<pepp::debug::Term>();
  } else if (auto open = std::get<Lit>(maybe_open); open.literal != '(') {
    return cp.rollback<pepp::debug::Term>();
  } else if (auto inner = parse_expression(tok); !inner) {
    return cp.rollback<pepp::debug::Term>();
  } else if (auto maybe_close = tok.match<Lit>(); !std::holds_alternative<Lit>(maybe_close)) {
    return cp.rollback<pepp::debug::Term>();
  } else if (auto close = std::get<Lit>(maybe_close); close.literal != ')') {
    // Invalid and we cannot recover.
    return cp.rollback<pepp::debug::Term>();
  } else {
    return accept(Parenthesized(inner));
  }
}

std::shared_ptr<pepp::debug::Constant> pepp::debug::Parser::parse_constant(TokenBuffer &tok) {
  using UC = detail::UnsignedConstant;
  if (auto maybe_constant = tok.match<UC>(); !std::holds_alternative<UC>(maybe_constant)) {
    return nullptr;
  } else {
    return accept(Constant(std::get<UC>(maybe_constant)));
  }
}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::compile(QStringView expr, void *builtins) {
  TokenBuffer tok(expr);
  return parse_p5(tok);
}

std::shared_ptr<pepp::debug::Expression> pepp::debug::compile(std::string_view expr, ExpressionCache &cache,
                                                              void *builtins) {
  return nullptr;
}
