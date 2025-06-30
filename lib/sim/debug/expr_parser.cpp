#include "expr_parser.hpp"

#include "expr_ast.hpp"

namespace pepp::debug::detail {

struct TokenCheckpoint;

struct TokenBuffer {
  explicit inline TokenBuffer(QStringView expr) : _lex(Lexer(expr)), _tokens(), _head(0) {}
  template <typename T> Lexer::Token match() {
    auto next = peek<T>();
    if (std::holds_alternative<T>(next)) _head++;
    return next;
  }
  template <typename T> Lexer::Token peek() {
    if (_head == _tokens.size()) {
      auto token = _lex.next_token();
      _tokens.emplace_back(token);
    }
    if (auto l = _tokens[_head]; std::holds_alternative<T>(l)) return l;
    return std::monostate{};
  }
  Lexer _lex;
  std::vector<Lexer::Token> _tokens;
  size_t _head = 0;
};

// Cache the results of each parsing rule to prevent exponentianl time complexity in the parser.
struct Memo {
  explicit Memo();
  explicit Memo(const TokenCheckpoint &cp, Parser::Rule r, std::shared_ptr<Term> t = nullptr);
  // Used in memo_cache to help with match_at. Initializes start to equal end, which makes the memo empty.
  // Then, it should compare less than any other element with the same (rule, start) for the sake of lower_bound.
  Memo(uint16_t start_end, Parser::Rule r);

  // Term is excluded from sorting, because it the payload and not part of the key.
  // Sorting on term WILL break usage in std::set.
  std::strong_ordering operator<=>(const Memo &rhs) const;
  // Makes printing with qDebug() easier.
  operator QString() const;

  Parser::Rule rule;   // To which parsing rule does this memo apply?
  uint16_t start, end; // Token range in TokenBuffer to which memo applies.
  // Pointer to the parsed result. Will be nullptr when trying to construct a key for MemoCache::match_at or to record a
  // failed match.
  // Must not be sorted on!!
  mutable std::shared_ptr<Term> term;
};

// Mapping from (rule, start index, end index) to Term, used to reduce parsing time complexity from exponential to
// polynomial, at the cost of quadratic memory usage. If match_at returns a nullptr, it must be interpeted as that
// rule has not been applied to a given position yet rather than evidence that the rule failed to parse at that
// position. This distinction is required so that this is a performance optimization rather than a correctness
// requirement.
struct MemoCache {
  // If the given token index has successfully parsed under a given rule before, return the AST node.
  // May be null, and only valid until next insert into memos set.
  // If null, assume that you re-parse under the given rule.
  std::tuple<std::shared_ptr<Term>, uint16_t> match_at(uint16_t position, Parser::Rule);
  void insert(Memo &&m);
  // Uses a set with a mutable data field because it was the first thing that came to mind.
  std::set<Memo> memos;
};

struct TokenCheckpoint {
  inline TokenCheckpoint(TokenBuffer &buf, MemoCache &cache) : _head(buf._head), _buf(buf), _cache(cache) {}
  template <typename T> std::shared_ptr<T> rollback([[maybe_unused]] Parser::Rule r) {
    _buf._head = _head;
    return nullptr;
  }
  // Record that a rule successfully parsed a region of tokens from the start of this to the current position of _buf.
  template <typename T> std::shared_ptr<T> memoize(std::shared_ptr<T> v, Parser::Rule r) {
    _cache.insert(Memo(*this, r, v));
    return v;
  }
  // Method to allow skipping over already-parsed token sequences.
  // Maybe this doesn't belong here and should be a free method, but it made for consistency in the parser having
  // all returns be fo the form `return cp...`
  template <typename T> std::shared_ptr<T> use_memo(std::shared_ptr<pepp::debug::Term> term, uint16_t end) {
    // If there was no match, do not consume tokens.
    if (term == nullptr) return nullptr;
    // Otherwise, consume tokens and type-cast the pointer.
    _buf._head = end;
    // Avoid casting from T to T.
    if constexpr (std::is_same_v<T, pepp::debug::Term>) return term;
    else return std::dynamic_pointer_cast<T>(term);
  }
  inline uint16_t start() const { return _head; }
  size_t _head = 0;
  TokenBuffer &_buf;
  MemoCache &_cache;
};

pepp::debug::detail::Memo::Memo() : rule(Parser::Rule::INVALID), start(0), end(0), term(nullptr) {}

pepp::debug::detail::Memo::Memo(uint16_t start_end, Parser::Rule r)
    : rule(r), start(start_end), end(start_end), term(nullptr) {}

pepp::debug::detail::Memo::Memo(const TokenCheckpoint &cp, Parser::Rule r, std::shared_ptr<Term> t)
    : rule(r), start(cp._head), end(cp._buf._head), term(t) {}

std::strong_ordering pepp::debug::detail::Memo::operator<=>(const Memo &rhs) const {
  if (auto cmp = rule <=> rhs.rule; cmp != 0) return cmp;
  else if (auto cmp = start <=> rhs.start; cmp != 0) return cmp;
  return end <=> rhs.end;
}

std::tuple<std::shared_ptr<pepp::debug::Term>, uint16_t> pepp::debug::detail::MemoCache::match_at(uint16_t position,
                                                                                                  Parser::Rule rule) {
  auto target = Memo(position, rule);
  auto lb = memos.lower_bound(target);
  // qDebug() << target << *lb;
  if (lb != memos.end() && lb->rule == rule && lb->start == position) return {lb->term, lb->end};
  return {nullptr, 0};
}

void pepp::debug::detail::MemoCache::insert(Memo &&m) { memos.insert(std::move(m)); }

pepp::debug::detail::Memo::operator QString() const {
  using namespace Qt::StringLiterals;
  return u"%1:[%2,%3)=%4"_s.arg((int)rule, 2).arg(start, 2).arg(end, 2).arg((uint64_t)&*term, 16, 16, QChar('0'));
}

} // namespace pepp::debug::detail

pepp::debug::Parser::Parser(ExpressionCache &cache) : _cache(cache) {}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::compile(QStringView expr, void *builtins) {
  detail::TokenBuffer tok(expr);
  detail::MemoCache cache{};
  auto ret = parse_p7(tok, cache);
  auto at_end = tok.peek<detail::Eof>();
  return (std::holds_alternative<std::monostate>(at_end)) ? nullptr : ret;
}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::compile(QString expr, void *builtins) {
  return compile(QStringView(expr), builtins);
}

std::shared_ptr<pepp::debug::DebuggerVariable> pepp::debug::Parser::parse_debug_identifier(detail::TokenBuffer &tok,
                                                                                           detail::MemoCache &cache) {
  using DBG = detail::DebugIdentifier;
  static const auto rule = Parser::Rule::DEBUG_IDENT;
  auto cp = detail::TokenCheckpoint(tok, cache);
  if (auto [term, end] = cache.match_at(cp.start(), rule); term != nullptr)
    return cp.use_memo<pepp::debug::DebuggerVariable>(term, end);
  if (auto maybe_debug_ident = tok.match<DBG>(); !std::holds_alternative<DBG>(maybe_debug_ident)) return nullptr;
  else return cp.memoize(accept(DebuggerVariable(std::get<DBG>(maybe_debug_ident))), rule);
}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_identifier_as_term(detail::TokenBuffer &tok,
                                                                                 detail::MemoCache &cache) {
  return parse_identifier(tok, cache);
}

std::shared_ptr<pepp::debug::Variable> pepp::debug::Parser::parse_identifier(detail::TokenBuffer &tok,
                                                                             detail::MemoCache &cache) {
  using ID = detail::Identifier;
  static const auto rule = Parser::Rule::IDENT;
  auto cp = detail::TokenCheckpoint(tok, cache);
  if (auto [term, end] = cache.match_at(cp.start(), rule); term != nullptr)
    return cp.use_memo<pepp::debug::Variable>(term, end);
  if (auto maybe_ident = tok.match<ID>(); !std::holds_alternative<ID>(maybe_ident)) return nullptr;
  else return cp.memoize(accept(Variable(std::get<ID>(maybe_ident))), rule);
}

std::shared_ptr<pepp::debug::Constant> pepp::debug::Parser::parse_constant(detail::TokenBuffer &tok,
                                                                           detail::MemoCache &cache) {
  using UC = detail::UnsignedConstant;
  using TYPE = detail::TypeSuffix;
  static const auto rule = Parser::Rule::CONSTANT;
  auto cp = detail::TokenCheckpoint(tok, cache);
  if (auto [term, end] = cache.match_at(cp.start(), rule); term != nullptr) cp.use_memo<pepp::debug::Term>(term, end);
  if (auto maybe_constant = tok.match<UC>(); std::holds_alternative<UC>(maybe_constant)) {
    auto as_constant = std::get<UC>(maybe_constant);
    if (auto maybe_trailing_type = tok.match<TYPE>(); std::holds_alternative<TYPE>(maybe_trailing_type)) {
      const auto type = std::get<TYPE>(maybe_trailing_type);
      auto bits = TypedBits{.allows_address_of = false, .type = type.type, .bits = as_constant.value};
      return cp.memoize(accept(Constant(bits, as_constant.format)), rule);
    } else {
      auto bits = TypedBits{.allows_address_of = false, .type = ExpressionType::i16, .bits = as_constant.value};
      return cp.memoize(accept(Constant(bits, as_constant.format)), rule);
    }
  }
  return nullptr;
}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_value(detail::TokenBuffer &tok,
                                                                    detail::MemoCache &cache) {
  static const auto rule = Parser::Rule::VALUE;
  auto cp = detail::TokenCheckpoint(tok, cache);
  if (auto [term, end] = cache.match_at(cp.start(), rule); term != nullptr)
    return cp.use_memo<pepp::debug::Term>(term, end);

  if (auto maybe_reg = parse_debug_identifier(tok, cache); maybe_reg != nullptr) return cp.memoize(maybe_reg, rule);
  else if (auto maybe_constant = parse_constant(tok, cache); maybe_constant != nullptr)
    return cp.memoize(maybe_constant, rule);
  else if (auto maybe_ident = parse_identifier(tok, cache); maybe_ident != nullptr)
    return cp.memoize(maybe_ident, rule);
  else if (auto maybe_par = parse_parened(tok, cache); maybe_par != nullptr) return cp.memoize(maybe_par, rule);
  return nullptr;
}


std::shared_ptr<pepp::debug::Term>
pepp::debug::Parser::parse_binary_infix(detail::TokenBuffer &tok, detail::MemoCache &cache,
                                        const std::set<BinaryInfix::Operators> &valid, Parser::ParseFn parse_lhs,
                                        Parser::ParseFn parse_rhs) {
  using LIT = detail::Literal;
  using ID = detail::Identifier;
  using Ops = BinaryInfix::Operators;
  auto cp = detail::TokenCheckpoint(tok, cache);

  // I'm sorry I'm not using std::invoke. Debugging nested binary infix parsing  (2 * s + 10) has been a pain.
  // std::invoke adds multiple extra unnecessary call frames in debug mode and makes stepping more difficult.
  // Sorry to the people at isocpp (https://isocpp.org/wiki/faq/pointers-to-members)
  auto maybe_lhs = (this->*parse_lhs)(tok, cache);
  if (maybe_lhs == nullptr) return cp.rollback<pepp::debug::Term>(Rule::INVALID);

  auto maybe_lit = tok.match<LIT>();
  if (!std::holds_alternative<LIT>(maybe_lit)) return cp.rollback<pepp::debug::Term>(Rule::INVALID);
  const auto &lit = std::get<LIT>(maybe_lit);
  auto op = string_to_binary_infix(lit.literal);
  if (!op) return cp.rollback<pepp::debug::Term>(Rule::INVALID);
  else if (!valid.contains(*op)) return cp.rollback<pepp::debug::Term>(Rule::INVALID);

  // Still sorry; see above.
  auto maybe_rhs = (this->*parse_rhs)(tok, cache);
  if (maybe_rhs == nullptr) return cp.rollback<pepp::debug::Term>(Rule::INVALID);

  return accept(BinaryInfix(*op, maybe_lhs, maybe_rhs));
}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_p0(detail::TokenBuffer &tok, detail::MemoCache &cache) {
  using Operators = BinaryInfix::Operators;
  static const auto valid = std::set<Operators>{Operators::STAR_DOT, Operators::DOT};
  static const auto rule = Parser::Rule::P0;
  auto cp = detail::TokenCheckpoint(tok, cache);
  if (auto [term, end] = cache.match_at(cp.start(), rule); term != nullptr)
    return cp.use_memo<pepp::debug::Term>(term, end);

  static const auto fn = &Parser::parse_identifier_as_term;
  if (auto maybe_member_access = parse_binary_infix(tok, cache, valid, fn, fn); maybe_member_access != nullptr)
    return cp.memoize(maybe_member_access, rule);
  else if (auto value = parse_value(tok, cache); value != nullptr) return cp.memoize(value, rule);
  return cp.rollback<pepp::debug::Term>(rule);
}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_p1(detail::TokenBuffer &tok, detail::MemoCache &cache) {
  using Lit = detail::Literal;
  using Cast = detail::TypeCast;
  static const auto rule = Parser::Rule::P1;
  auto cp = detail::TokenCheckpoint(tok, cache);
  if (auto [term, end] = cache.match_at(cp.start(), rule); term != nullptr)
    return cp.use_memo<pepp::debug::Term>(term, end);

  if (auto maybe_prefix = tok.match<Lit>(); std::holds_alternative<Lit>(maybe_prefix)) {
    const auto &lit = std::get<Lit>(maybe_prefix);
    auto op = string_to_unary_prefix(lit.literal);
    // Changed from return to enable p0 to evaluate parenthetical expressions.
    if (!op) cp.rollback<pepp::debug::Term>(rule);
    else {
      auto arg = parse_p0(tok, cache);
      if (arg == nullptr) return cp.rollback<pepp::debug::Term>(rule);
      return cp.memoize(accept(UnaryPrefix(*op, arg)), rule);
    }
  } else if (auto maybe_cast = tok.match<Cast>(); std::holds_alternative<Cast>(maybe_cast)) {
    const auto &cast = std::get<Cast>(maybe_cast);
    auto arg = parse_p0(tok, cache);
    if (arg == nullptr) return cp.rollback<pepp::debug::Term>(rule);
    return cp.memoize(accept(ExplicitCast(cast.type, arg)), rule);
  }
  return parse_p0(tok, cache);
}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_p2(detail::TokenBuffer &tok, detail::MemoCache &cache) {
  using Operators = BinaryInfix::Operators;
  static const auto valid = std::set<Operators>{Operators::MULTIPLY, Operators::DIVIDE, Operators::MODULO};
  static const auto rule = Parser::Rule::P2;
  auto cp = detail::TokenCheckpoint(tok, cache);
  if (auto [term, end] = cache.match_at(cp.start(), rule); term != nullptr)
    return cp.use_memo<pepp::debug::Term>(term, end);

  if (auto maybe_infix = parse_binary_infix(tok, cache, valid, &Parser::parse_p1, &Parser::parse_p2);
      maybe_infix != nullptr)
    return cp.memoize(maybe_infix, rule);
  else if (auto maybe_value = parse_p1(tok, cache); maybe_value != nullptr) return cp.memoize(maybe_value, rule);
  return cp.rollback<pepp::debug::Term>(rule);
}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_p3(detail::TokenBuffer &tok, detail::MemoCache &cache) {
  using Operators = BinaryInfix::Operators;
  static const auto valid = std::set<Operators>{Operators::ADD, Operators::SUBTRACT};
  static const auto rule = Parser::Rule::P3;
  auto cp = detail::TokenCheckpoint(tok, cache);
  if (auto [term, end] = cache.match_at(cp.start(), rule); term != nullptr)
    return cp.use_memo<pepp::debug::Term>(term, end);

  if (auto maybe_infix = parse_binary_infix(tok, cache, valid, &Parser::parse_p2, &Parser::parse_p3);
      maybe_infix != nullptr)
    return cp.memoize(maybe_infix, rule);
  else if (auto maybe_value = parse_p2(tok, cache); maybe_value != nullptr) return cp.memoize(maybe_value, rule);
  return cp.rollback<pepp::debug::Term>(rule);
}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_p4(detail::TokenBuffer &tok, detail::MemoCache &cache) {
  using Operators = BinaryInfix::Operators;
  static const auto valid = std::set<Operators>{Operators::SHIFT_LEFT, Operators::SHIFT_RIGHT};
  static const auto rule = Parser::Rule::P4;
  auto cp = detail::TokenCheckpoint(tok, cache);
  if (auto [term, end] = cache.match_at(cp.start(), rule); term != nullptr)
    return cp.use_memo<pepp::debug::Term>(term, end);

  if (auto maybe_infix = parse_binary_infix(tok, cache, valid, &Parser::parse_p3, &Parser::parse_p4);
      maybe_infix != nullptr)
    return cp.memoize(maybe_infix, rule);
  else if (auto maybe_value = parse_p3(tok, cache); maybe_value != nullptr) return cp.memoize(maybe_value, rule);
  return cp.rollback<pepp::debug::Term>(rule);
}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_p5(detail::TokenBuffer &tok, detail::MemoCache &cache) {
  using Operators = BinaryInfix::Operators;
  static const auto valid =
      std::set<Operators>{Operators::LESS, Operators::LESS_OR_EQUAL, Operators::GREATER_OR_EQUAL, Operators::GREATER};
  static const auto rule = Parser::Rule::P5;
  auto cp = detail::TokenCheckpoint(tok, cache);
  if (auto [term, end] = cache.match_at(cp.start(), rule); term != nullptr)
    return cp.use_memo<pepp::debug::Term>(term, end);

  if (auto maybe_infix = parse_binary_infix(tok, cache, valid, &Parser::parse_p4, &Parser::parse_p5);
      maybe_infix != nullptr)
    return cp.memoize(maybe_infix, rule);
  else if (auto maybe_value = parse_p4(tok, cache); maybe_value != nullptr) return cp.memoize(maybe_value, rule);
  return cp.rollback<pepp::debug::Term>(rule);
}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_p6(detail::TokenBuffer &tok, detail::MemoCache &cache) {
  using Operators = BinaryInfix::Operators;
  static const auto valid = std::set<Operators>{Operators::EQUAL, Operators::NOT_EQUAL};
  static const auto rule = Parser::Rule::P6;
  auto cp = detail::TokenCheckpoint(tok, cache);
  if (auto [term, end] = cache.match_at(cp.start(), rule); term != nullptr)
    return cp.use_memo<pepp::debug::Term>(term, end);

  if (auto maybe_infix = parse_binary_infix(tok, cache, valid, &Parser::parse_p5, &Parser::parse_p6);
      maybe_infix != nullptr)
    return cp.memoize(maybe_infix, rule);
  else if (auto maybe_value = parse_p5(tok, cache); maybe_value != nullptr) return cp.memoize(maybe_value, rule);
  return cp.rollback<pepp::debug::Term>(rule);
}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_p7(detail::TokenBuffer &tok, detail::MemoCache &cache) {
  using Operators = BinaryInfix::Operators;
  static const auto valid = std::set<Operators>{Operators::BIT_AND, Operators::BIT_OR, Operators::BIT_XOR};
  static const auto rule = Parser::Rule::P7;
  auto cp = detail::TokenCheckpoint(tok, cache);
  if (auto [term, end] = cache.match_at(cp.start(), rule); term != nullptr)
    return cp.use_memo<pepp::debug::Term>(term, end);

  if (auto maybe_infix = parse_binary_infix(tok, cache, valid, &Parser::parse_p6, &Parser::parse_p7);
      maybe_infix != nullptr)
    return cp.memoize(maybe_infix, rule);
  else if (auto maybe_value = parse_p6(tok, cache); maybe_value != nullptr) return cp.memoize(maybe_value, rule);
  return cp.rollback<pepp::debug::Term>(rule);
}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_parened(detail::TokenBuffer &tok,
                                                                      detail::MemoCache &cache) {
  using Lit = detail::Literal;
  static const auto rule = Parser::Rule::PAREN;
  auto cp = detail::TokenCheckpoint(tok, cache);
  if (auto [term, end] = cache.match_at(cp.start(), rule); term != nullptr)
    return cp.use_memo<pepp::debug::Term>(term, end);

  if (auto maybe_open = tok.match<Lit>(); !std::holds_alternative<Lit>(maybe_open))
    return cp.rollback<pepp::debug::Term>(rule);
  else if (const auto &open = std::get<Lit>(maybe_open); open.literal != '(')
    return cp.rollback<pepp::debug::Term>(rule);
  else if (auto inner = parse_p7(tok, cache); !inner) return cp.rollback<pepp::debug::Term>(rule);
  else if (auto maybe_close = tok.match<Lit>(); !std::holds_alternative<Lit>(maybe_close))
    return cp.rollback<pepp::debug::Term>(rule);
  else if (const auto &close = std::get<Lit>(maybe_close); close.literal != ')')
    return cp.rollback<pepp::debug::Term>(rule); // Invalid and we cannot recover.
  else return cp.memoize(accept(Parenthesized(inner)), rule);
}

