#include "expr_parser.hpp"

#include "expr_ast.hpp"

pepp::debug::Parser::Parser(ExpressionCache &cache) : _cache(cache) {}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_identifier_as_term(TokenBuffer &tok, MemoCache &cache) {
  return parse_identifier(tok, cache);
}

std::shared_ptr<pepp::debug::Variable> pepp::debug::Parser::parse_identifier(TokenBuffer &tok, MemoCache &cache) {
  using ID = detail::Identifier;
  static const auto rule = Parser::Rule::IDENT;
  auto cp = TokenCheckpoint(tok, cache);
  if (auto [term, end] = cache.match_at(cp.start(), rule); term != nullptr)
    return cp.use_memo<pepp::debug::Variable>(term, end);
  if (auto maybe_ident = tok.match<ID>(); !std::holds_alternative<ID>(maybe_ident)) return nullptr;
  else return cp.memoize(accept(Variable(std::get<ID>(maybe_ident))), rule);
}
std::shared_ptr<pepp::debug::Register> pepp::debug::Parser::parse_register(TokenBuffer &tok, MemoCache &cache) {
  return nullptr;
}

std::shared_ptr<pepp::debug::Constant> pepp::debug::Parser::parse_constant(TokenBuffer &tok, MemoCache &cache) {
  using UC = detail::UnsignedConstant;
  static const auto rule = Parser::Rule::CONSTANT;
  auto cp = TokenCheckpoint(tok, cache);
  if (auto [term, end] = cache.match_at(cp.start(), rule); term != nullptr) cp.use_memo<pepp::debug::Term>(term, end);
  if (auto maybe_constant = tok.match<UC>(); !std::holds_alternative<UC>(maybe_constant)) return nullptr;
  else {
    auto as_constant = std::get<UC>(maybe_constant);
    auto bits = TypedBits{.allows_address_of = false, .type = ExpressionType::i16, .bits = as_constant.value};
    return cp.memoize(accept(Constant(bits, as_constant.format)), rule);
  }
}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_value(TokenBuffer &tok, MemoCache &cache) {
  static const auto rule = Parser::Rule::VALUE;
  auto cp = TokenCheckpoint(tok, cache);
  if (auto [term, end] = cache.match_at(cp.start(), rule); term != nullptr)
    return cp.use_memo<pepp::debug::Term>(term, end);

  if (auto maybe_reg = parse_register(tok, cache); maybe_reg != nullptr) return cp.memoize(maybe_reg, rule);
  else if (auto maybe_constant = parse_constant(tok, cache); maybe_constant != nullptr)
    return cp.memoize(maybe_constant, rule);
  else if (auto maybe_ident = parse_identifier(tok, cache); maybe_ident != nullptr)
    return cp.memoize(maybe_ident, rule);
  else if (auto maybe_par = parse_parened(tok, cache); maybe_par != nullptr) return cp.memoize(maybe_par, rule);
  return nullptr;
}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_expression(TokenBuffer &tok, MemoCache &cache) {
  return nullptr;
}

std::shared_ptr<pepp::debug::Term>
pepp::debug::Parser::parse_binary_infix(TokenBuffer &tok, MemoCache &cache,
                                        const std::set<BinaryInfix::Operators> &valid, Parser::ParseFn parse_lhs,
                                        Parser::ParseFn parse_rhs) {
  using LIT = detail::Literal;
  using ID = detail::Identifier;
  using Ops = BinaryInfix::Operators;
  auto cp = TokenCheckpoint(tok, cache);

  // I'm sorry I'm not using std::invoke. Debugging nested binary infix parsing  (2 * s + 10) has been a pain.
  // std::invoke adds multiple extra unnecessary call frames in debug mode and makes stepping more difficult.
  // Sorry to the people at isocpp (https://isocpp.org/wiki/faq/pointers-to-members)
  auto maybe_lhs = (this->*parse_lhs)(tok, cache);
  if (maybe_lhs == nullptr) return cp.rollback<pepp::debug::Term>(Rule::INVALID);

  auto maybe_lit = tok.match<LIT>();
  if (!std::holds_alternative<LIT>(maybe_lit)) return cp.rollback<pepp::debug::Term>(Rule::INVALID);
  auto lit = std::get<LIT>(maybe_lit);
  auto op = string_to_binary_infix(lit.literal);
  if (!op) return cp.rollback<pepp::debug::Term>(Rule::INVALID);
  else if (!valid.contains(*op)) return cp.rollback<pepp::debug::Term>(Rule::INVALID);

  // Still sorry; see above.
  auto maybe_rhs = (this->*parse_rhs)(tok, cache);
  if (maybe_rhs == nullptr) return cp.rollback<pepp::debug::Term>(Rule::INVALID);

  return accept(BinaryInfix(*op, maybe_lhs, maybe_rhs));
}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_p0(TokenBuffer &tok, MemoCache &cache) {
  using Operators = BinaryInfix::Operators;
  static const auto valid = std::set<Operators>{Operators::STAR_DOT, Operators::DOT};
  static const auto rule = Parser::Rule::P0;
  auto cp = TokenCheckpoint(tok, cache);
  if (auto [term, end] = cache.match_at(cp.start(), rule); term != nullptr)
    return cp.use_memo<pepp::debug::Term>(term, end);

  static const auto fn = &Parser::parse_identifier_as_term;
  if (auto maybe_member_access = parse_binary_infix(tok, cache, valid, fn, fn); maybe_member_access != nullptr)
    return cp.memoize(maybe_member_access, rule);
  else if (auto value = parse_value(tok, cache); value != nullptr) return cp.memoize(value, rule);
  return cp.rollback<pepp::debug::Term>(rule);
}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_p1(TokenBuffer &tok, MemoCache &cache) {
  using Lit = detail::Literal;
  static const auto rule = Parser::Rule::P1;
  auto cp = TokenCheckpoint(tok, cache);
  if (auto [term, end] = cache.match_at(cp.start(), rule); term != nullptr)
    return cp.use_memo<pepp::debug::Term>(term, end);

  if (auto maybe_prefix = tok.match<Lit>(); std::holds_alternative<Lit>(maybe_prefix)) {
    auto lit = std::get<Lit>(maybe_prefix);
    auto op = string_to_unary_prefix(lit.literal);
    // Changed from return to enable p0 to evaluate parenthetical expressions.
    if (!op) cp.rollback<pepp::debug::Term>(rule);
    else {
      auto arg = parse_p0(tok, cache);
      if (arg == nullptr) return cp.rollback<pepp::debug::Term>(rule);
      return cp.memoize(accept(UnaryPrefix(*op, arg)), rule);
    }
  }
  return parse_p0(tok, cache);
}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_p2(TokenBuffer &tok, MemoCache &cache) {
  using Operators = BinaryInfix::Operators;
  static const auto valid = std::set<Operators>{Operators::MULTIPLY, Operators::DIVIDE, Operators::MODULO};
  static const auto rule = Parser::Rule::P2;
  auto cp = TokenCheckpoint(tok, cache);
  if (auto [term, end] = cache.match_at(cp.start(), rule); term != nullptr)
    return cp.use_memo<pepp::debug::Term>(term, end);

  if (auto maybe_infix = parse_binary_infix(tok, cache, valid, &Parser::parse_p1, &Parser::parse_p2);
      maybe_infix != nullptr)
    return cp.memoize(maybe_infix, rule);
  else if (auto maybe_value = parse_p1(tok, cache); maybe_value != nullptr) return cp.memoize(maybe_value, rule);
  return cp.rollback<pepp::debug::Term>(rule);
}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_p3(TokenBuffer &tok, MemoCache &cache) {
  using Operators = BinaryInfix::Operators;
  static const auto valid = std::set<Operators>{Operators::ADD, Operators::SUBTRACT};
  static const auto rule = Parser::Rule::P3;
  auto cp = TokenCheckpoint(tok, cache);
  if (auto [term, end] = cache.match_at(cp.start(), rule); term != nullptr)
    return cp.use_memo<pepp::debug::Term>(term, end);

  if (auto maybe_infix = parse_binary_infix(tok, cache, valid, &Parser::parse_p2, &Parser::parse_p3);
      maybe_infix != nullptr)
    return cp.memoize(maybe_infix, rule);
  else if (auto maybe_value = parse_p2(tok, cache); maybe_value != nullptr) return cp.memoize(maybe_value, rule);
  return cp.rollback<pepp::debug::Term>(rule);
}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_p4(TokenBuffer &tok, MemoCache &cache) {
  using Operators = BinaryInfix::Operators;
  static const auto valid = std::set<Operators>{Operators::SHIFT_LEFT, Operators::SHIFT_RIGHT};
  static const auto rule = Parser::Rule::P4;
  auto cp = TokenCheckpoint(tok, cache);
  if (auto [term, end] = cache.match_at(cp.start(), rule); term != nullptr)
    return cp.use_memo<pepp::debug::Term>(term, end);

  if (auto maybe_infix = parse_binary_infix(tok, cache, valid, &Parser::parse_p3, &Parser::parse_p4);
      maybe_infix != nullptr)
    return cp.memoize(maybe_infix, rule);
  else if (auto maybe_value = parse_p3(tok, cache); maybe_value != nullptr) return cp.memoize(maybe_value, rule);
  return cp.rollback<pepp::debug::Term>(rule);
}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_p5(TokenBuffer &tok, MemoCache &cache) {
  using Operators = BinaryInfix::Operators;
  static const auto valid =
      std::set<Operators>{Operators::LESS, Operators::LESS_OR_EQUAL, Operators::GREATER_OR_EQUAL, Operators::GREATER};
  static const auto rule = Parser::Rule::P5;
  auto cp = TokenCheckpoint(tok, cache);
  if (auto [term, end] = cache.match_at(cp.start(), rule); term != nullptr)
    return cp.use_memo<pepp::debug::Term>(term, end);

  if (auto maybe_infix = parse_binary_infix(tok, cache, valid, &Parser::parse_p4, &Parser::parse_p5);
      maybe_infix != nullptr)
    return cp.memoize(maybe_infix, rule);
  else if (auto maybe_value = parse_p4(tok, cache); maybe_value != nullptr) return cp.memoize(maybe_value, rule);
  return cp.rollback<pepp::debug::Term>(rule);
}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_p6(TokenBuffer &tok, MemoCache &cache) {
  using Operators = BinaryInfix::Operators;
  static const auto valid = std::set<Operators>{Operators::EQUAL, Operators::NOT_EQUAL};
  static const auto rule = Parser::Rule::P6;
  auto cp = TokenCheckpoint(tok, cache);
  if (auto [term, end] = cache.match_at(cp.start(), rule); term != nullptr)
    return cp.use_memo<pepp::debug::Term>(term, end);

  if (auto maybe_infix = parse_binary_infix(tok, cache, valid, &Parser::parse_p5, &Parser::parse_p6);
      maybe_infix != nullptr)
    return cp.memoize(maybe_infix, rule);
  else if (auto maybe_value = parse_p5(tok, cache); maybe_value != nullptr) return cp.memoize(maybe_value, rule);
  return cp.rollback<pepp::debug::Term>(rule);
}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_p7(TokenBuffer &tok, MemoCache &cache) {
  using Operators = BinaryInfix::Operators;
  static const auto valid = std::set<Operators>{Operators::BIT_AND, Operators::BIT_OR, Operators::BIT_XOR};
  static const auto rule = Parser::Rule::P7;
  auto cp = TokenCheckpoint(tok, cache);
  if (auto [term, end] = cache.match_at(cp.start(), rule); term != nullptr)
    return cp.use_memo<pepp::debug::Term>(term, end);

  if (auto maybe_infix = parse_binary_infix(tok, cache, valid, &Parser::parse_p6, &Parser::parse_p7);
      maybe_infix != nullptr)
    return cp.memoize(maybe_infix, rule);
  else if (auto maybe_value = parse_p6(tok, cache); maybe_value != nullptr) return cp.memoize(maybe_value, rule);
  return cp.rollback<pepp::debug::Term>(rule);
}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_parened(TokenBuffer &tok, MemoCache &cache) {
  using Lit = detail::Literal;
  static const auto rule = Parser::Rule::PAREN;
  auto cp = TokenCheckpoint(tok, cache);
  if (auto [term, end] = cache.match_at(cp.start(), rule); term != nullptr)
    return cp.use_memo<pepp::debug::Term>(term, end);

  if (auto maybe_open = tok.match<Lit>(); !std::holds_alternative<Lit>(maybe_open))
    return cp.rollback<pepp::debug::Term>(rule);
  else if (auto open = std::get<Lit>(maybe_open); open.literal != '(') return cp.rollback<pepp::debug::Term>(rule);
  else if (auto inner = parse_p7(tok, cache); !inner) return cp.rollback<pepp::debug::Term>(rule);
  else if (auto maybe_close = tok.match<Lit>(); !std::holds_alternative<Lit>(maybe_close))
    return cp.rollback<pepp::debug::Term>(rule);
  else if (auto close = std::get<Lit>(maybe_close); close.literal != ')')
    return cp.rollback<pepp::debug::Term>(rule); // Invalid and we cannot recover.
  else return cp.memoize(accept(Parenthesized(inner)), rule);
}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::compile(QStringView expr, void *builtins) {
  TokenBuffer tok(expr);
  MemoCache cache{};
  auto ret = parse_p7(tok, cache);
  auto at_end = tok.peek<detail::Eof>();
  return (std::holds_alternative<std::monostate>(at_end)) ? nullptr : ret;
}

std::shared_ptr<pepp::debug::Expression> pepp::debug::compile(std::string_view expr, ExpressionCache &cache,
                                                              void *builtins) {
  return nullptr;
}

pepp::debug::Memo::Memo() : rule(Parser::Rule::INVALID), start(0), end(0), term(nullptr) {}

pepp::debug::Memo::Memo(uint16_t start_end, Parser::Rule r)
    : rule(r), start(start_end), end(start_end), term(nullptr) {}

pepp::debug::Memo::Memo(const TokenCheckpoint &cp, Parser::Rule r, std::shared_ptr<Term> t)
    : rule(r), start(cp._head), end(cp._buf._head), term(t) {}

std::strong_ordering pepp::debug::Memo::operator<=>(const Memo &rhs) const {
  if (auto cmp = rule <=> rhs.rule; cmp != 0) return cmp;
  else if (auto cmp = start <=> rhs.start; cmp != 0) return cmp;
  return end <=> rhs.end;
}

std::tuple<std::shared_ptr<pepp::debug::Term>, uint16_t> pepp::debug::MemoCache::match_at(uint16_t position,
                                                                                          Parser::Rule rule) {
  auto target = Memo(position, rule);
  auto lb = memos.lower_bound(target);
  // qDebug() << target << *lb;
  if (lb != memos.end() && lb->rule == rule && lb->start == position) return {lb->term, lb->end};
  return {nullptr, 0};
}

void pepp::debug::MemoCache::insert(Memo &&m) { memos.insert(m); }

pepp::debug::Memo::operator QString() const {
  using namespace Qt::StringLiterals;
  return u"%1:[%2,%3)=%4"_s.arg((int)rule, 2).arg(start, 2).arg(end, 2).arg((uint64_t)&*term, 16, 16, QChar('0'));
}
