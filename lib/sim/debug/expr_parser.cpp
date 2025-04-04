#include "expr_parser.hpp"

#include "expr_ast.hpp"

pepp::debug::Parser::Parser(ExpressionCache &cache) : _cache(cache) {}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_identifier_as_term(TokenBuffer &tok) {
  return parse_identifier(tok);
}

std::shared_ptr<pepp::debug::Variable> pepp::debug::Parser::parse_identifier(TokenBuffer &tok) {
  using ID = detail::Identifier;
  auto cp = TokenCheckpoint(tok);
  if (auto maybe_ident = tok.match<ID>(); !std::holds_alternative<ID>(maybe_ident)) return nullptr;
  else return cp.memoize(accept(Variable(std::get<ID>(maybe_ident))), Rule::IDENT);
}
std::shared_ptr<pepp::debug::Register> pepp::debug::Parser::parse_register(TokenBuffer &tok) { return nullptr; }

std::shared_ptr<pepp::debug::Constant> pepp::debug::Parser::parse_constant(TokenBuffer &tok) {
  using UC = detail::UnsignedConstant;
  auto cp = TokenCheckpoint(tok);
  if (auto maybe_constant = tok.match<UC>(); !std::holds_alternative<UC>(maybe_constant)) return nullptr;
  else return cp.memoize(accept(Constant(std::get<UC>(maybe_constant))), Rule::CONSTANT);
}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_value(TokenBuffer &tok) {
  if (auto maybe_reg = parse_register(tok); maybe_reg != nullptr) return maybe_reg;
  else if (auto maybe_constant = parse_constant(tok); maybe_constant != nullptr) return maybe_constant;
  else if (auto maybe_ident = parse_identifier(tok); maybe_ident != nullptr) return maybe_ident;
  return nullptr;
}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_expression(TokenBuffer &tok) { return nullptr; }

std::shared_ptr<pepp::debug::Term>
pepp::debug::Parser::parse_binary_infix(TokenBuffer &tok, const std::set<BinaryInfix::Operators> &valid,
                                        Parser::ParseFn parse) {
  using LIT = detail::Literal;
  using ID = detail::Identifier;
  using Ops = BinaryInfix::Operators;
  auto cp = TokenCheckpoint(tok);

  // I'm sorry I'm not using std::invoke. Debugging nested binary infix parsing  (2 * s + 10) has been a pain.
  // std::invoke adds multiple extra unnecessary call frames in debug mode and makes stepping more difficult.
  // Sorry to the people at isocpp (https://isocpp.org/wiki/faq/pointers-to-members)
  auto maybe_lhs = (this->*parse)(tok);
  if (maybe_lhs == nullptr) return cp.rollback<pepp::debug::Term>(Rule::INVALID);

  auto maybe_lit = tok.match<LIT>();
  if (!std::holds_alternative<LIT>(maybe_lit)) return cp.rollback<pepp::debug::Term>(Rule::INVALID);
  auto lit = std::get<LIT>(maybe_lit);
  auto op = string_to_binary_infix(lit.literal);
  if (!op) return cp.rollback<pepp::debug::Term>(Rule::INVALID);
  else if (!valid.contains(*op)) return cp.rollback<pepp::debug::Term>(Rule::INVALID);

  // Still sorry; see above.
  auto maybe_rhs = (this->*parse)(tok);
  if (maybe_rhs == nullptr) return cp.rollback<pepp::debug::Term>(Rule::INVALID);

  return accept(BinaryInfix(*op, maybe_lhs, maybe_rhs));
}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_p0(TokenBuffer &tok) {
  using Operators = BinaryInfix::Operators;
  static const auto valid = std::set<Operators>{Operators::STAR_DOT, Operators::DOT};
  static const auto rule = Parser::Rule::P0;
  auto cp = TokenCheckpoint(tok);
  if (auto maybe_member_access = parse_binary_infix(tok, valid, &Parser::parse_identifier_as_term);
      maybe_member_access != nullptr)
    return cp.memoize(maybe_member_access, rule);
  else if (auto value = parse_value(tok); value != nullptr) return cp.memoize(value, rule);
  return cp.rollback<pepp::debug::Term>(rule);
}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_p1(TokenBuffer &tok) {
  using Lit = detail::Literal;
  static const auto rule = Parser::Rule::P1;
  auto cp = TokenCheckpoint(tok);
  if (auto maybe_prefix = tok.match<Lit>(); std::holds_alternative<Lit>(maybe_prefix)) {
    auto lit = std::get<Lit>(maybe_prefix);
    auto op = string_to_unary_prefix(lit.literal);
    if (!op) return cp.rollback<pepp::debug::Term>(rule);
    auto arg = parse_p0(tok);
    if (arg == nullptr) return cp.rollback<pepp::debug::Term>(rule);
    return cp.memoize(accept(UnaryPrefix(*op, arg)), rule);
  }
  return parse_p0(tok);
}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_p2(TokenBuffer &tok) {
  using Operators = BinaryInfix::Operators;
  static const auto valid = std::set<Operators>{Operators::MULTIPLY, Operators::DIVIDE, Operators::MODULO};
  static const auto rule = Parser::Rule::P2;
  auto cp = TokenCheckpoint(tok);
  if (auto maybe_infix = parse_binary_infix(tok, valid, &Parser::parse_p1); maybe_infix != nullptr)
    return cp.memoize(maybe_infix, rule);
  else if (auto maybe_value = parse_p1(tok); maybe_value != nullptr) return cp.memoize(maybe_value, rule);
  return cp.rollback<pepp::debug::Term>(rule);
}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_p3(TokenBuffer &tok) {
  using Operators = BinaryInfix::Operators;
  static const auto valid = std::set<Operators>{Operators::ADD, Operators::SUBTRACT};
  static const auto rule = Parser::Rule::P3;
  auto cp = TokenCheckpoint(tok);
  if (auto maybe_infix = parse_binary_infix(tok, valid, &Parser::parse_p2); maybe_infix != nullptr)
    return cp.memoize(maybe_infix, rule);
  else if (auto maybe_value = parse_p2(tok); maybe_value != nullptr) return cp.memoize(maybe_value, rule);
  return cp.rollback<pepp::debug::Term>(rule);
}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_p4(TokenBuffer &tok) {
  using Operators = BinaryInfix::Operators;
  static const auto valid = std::set<Operators>{Operators::SHIFT_LEFT, Operators::SHIFT_RIGHT};
  static const auto rule = Parser::Rule::P4;
  auto cp = TokenCheckpoint(tok);
  if (auto maybe_infix = parse_binary_infix(tok, valid, &Parser::parse_p3); maybe_infix != nullptr)
    return cp.memoize(maybe_infix, rule);
  else if (auto maybe_value = parse_p3(tok); maybe_value != nullptr) return cp.memoize(maybe_value, rule);
  return cp.rollback<pepp::debug::Term>(rule);
}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_p5(TokenBuffer &tok) {
  using Operators = BinaryInfix::Operators;
  static const auto valid =
      std::set<Operators>{Operators::LESS, Operators::LESS_OR_EQUAL, Operators::GREATER_OR_EQUAL, Operators::GREATER};
  static const auto rule = Parser::Rule::P5;
  auto cp = TokenCheckpoint(tok);
  if (auto maybe_infix = parse_binary_infix(tok, valid, &Parser::parse_p4); maybe_infix != nullptr)
    return cp.memoize(maybe_infix, rule);
  else if (auto maybe_value = parse_p4(tok); maybe_value != nullptr) return cp.memoize(maybe_value, rule);
  return cp.rollback<pepp::debug::Term>(rule);
}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_p6(TokenBuffer &tok) {
  using Operators = BinaryInfix::Operators;
  static const auto valid = std::set<Operators>{Operators::EQUAL, Operators::NOT_EQUAL};
  static const auto rule = Parser::Rule::P6;
  auto cp = TokenCheckpoint(tok);
  if (auto maybe_infix = parse_binary_infix(tok, valid, &Parser::parse_p5); maybe_infix != nullptr)
    return cp.memoize(maybe_infix, rule);
  else if (auto maybe_value = parse_p5(tok); maybe_value != nullptr) return cp.memoize(maybe_value, rule);
  return cp.rollback<pepp::debug::Term>(rule);
}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_p7(TokenBuffer &tok) {
  using Operators = BinaryInfix::Operators;
  static const auto valid = std::set<Operators>{Operators::BIT_AND, Operators::BIT_OR, Operators::BIT_XOR};
  static const auto rule = Parser::Rule::P7;
  auto cp = TokenCheckpoint(tok);
  if (auto maybe_infix = parse_binary_infix(tok, valid, &Parser::parse_p6); maybe_infix != nullptr)
    return cp.memoize(maybe_infix, rule);
  else if (auto maybe_value = parse_p6(tok); maybe_value != nullptr) return cp.memoize(maybe_value, rule);
  return cp.rollback<pepp::debug::Term>(rule);
}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_parened(TokenBuffer &tok) {
  using Lit = detail::Literal;
  static const auto rule = Parser::Rule::PAREN;
  auto cp = TokenCheckpoint(tok);
  if (auto maybe_open = tok.match<Lit>(); !std::holds_alternative<Lit>(maybe_open))
    return cp.rollback<pepp::debug::Term>(rule);
  else if (auto open = std::get<Lit>(maybe_open); open.literal != '(') return cp.rollback<pepp::debug::Term>(rule);
  else if (auto inner = parse_expression(tok); !inner) return cp.rollback<pepp::debug::Term>(rule);
  else if (auto maybe_close = tok.match<Lit>(); !std::holds_alternative<Lit>(maybe_close))
    return cp.rollback<pepp::debug::Term>(rule);
  else if (auto close = std::get<Lit>(maybe_close); close.literal != ')')
    return cp.rollback<pepp::debug::Term>(rule); // Invalid and we cannot recover.
  else return cp.memoize(accept(Parenthesized(inner)), rule);
}


std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::compile(QStringView expr, void *builtins) {
  TokenBuffer tok(expr);
  auto ret = parse_p7(tok);
  auto at_end = tok.peek<detail::Eof>();
  return (std::holds_alternative<std::monostate>(at_end)) ? nullptr : ret;
}

std::shared_ptr<pepp::debug::Expression> pepp::debug::compile(std::string_view expr, ExpressionCache &cache,
                                                              void *builtins) {
  return nullptr;
}

pepp::debug::Parser::Memo::Memo() : rule(Rule::INVALID), start(0), end(0), term(nullptr) {}

pepp::debug::Parser::Memo::Memo(const TokenCheckpoint &cp, Rule r)
    : rule(r), start(cp._head), end(cp._buf._head), term(nullptr) {}
