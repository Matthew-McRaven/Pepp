#pragma once
#include <QtCore>
#include <set>
#include "expr_ast.hpp"
#include "expr_tokenizer.hpp"

namespace pepp::debug {

struct ExpressionCache {
  struct Compare {
    using is_transparent = void;
    bool operator()(const std::shared_ptr<Term> &lhs, const std::shared_ptr<Term> &rhs) const { return *lhs < *rhs; }
    bool operator()(const Term &lhs, const std::shared_ptr<Term> &rhs) const { return lhs < *rhs; }
    bool operator()(const std::shared_ptr<Term> &lhs, const Term &rhs) const { return *lhs < rhs; }
    bool operator()(const Term &lhs, const Term &rhs) const { return lhs < rhs; }
  };
  using Set = std::set<std::shared_ptr<Term>, Compare>;
  Set _set{};

  template <typename T> std::shared_ptr<T> add_or_return(T &&item) {
    Set::iterator search = _set.find(item);
    if (search == _set.end()) {
      auto ret = std::make_shared<T>(std::forward<T>(item));
      // Set up dependent tracking on creation.
      ret->link();
      _set.insert(ret);
      return ret;
    } else return std::dynamic_pointer_cast<T>(*search);
  }
};

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

struct TokenCheckpoint;
struct MemoCache;

struct Parser {
  // Helper to associaite regions with parsing functions in Memo/MemoCache
  enum class Rule : uint8_t {
    REGISTER,
    VALUE,
    PAREN,
    CONSTANT,
    IDENT,
    P0,
    P1,
    P2,
    P3,
    P4,
    P5,
    P6,
    P7,
    EXPRESSION,
    INVALID
  };

  explicit Parser(ExpressionCache &cache);
  std::shared_ptr<Register> parse_register(TokenBuffer &tok, MemoCache &cache);
  std::shared_ptr<Term> parse_value(TokenBuffer &tok, MemoCache &cache);
  std::shared_ptr<Term> parse_parened(TokenBuffer &tok, MemoCache &cache);
  std::shared_ptr<Constant> parse_constant(TokenBuffer &tok, MemoCache &cache);
  std::shared_ptr<Variable> parse_identifier(TokenBuffer &tok, MemoCache &cache);
  std::shared_ptr<Term> parse_p7(TokenBuffer &tok, MemoCache &cache); // Bitwise ops
  std::shared_ptr<Term> parse_p6(TokenBuffer &tok, MemoCache &cache); // Equality
  std::shared_ptr<Term> parse_p5(TokenBuffer &tok, MemoCache &cache); // Inequality
  std::shared_ptr<Term> parse_p4(TokenBuffer &tok, MemoCache &cache); // Bitwise shifts
  std::shared_ptr<Term> parse_p3(TokenBuffer &tok, MemoCache &cache); // +-
  std::shared_ptr<Term> parse_p2(TokenBuffer &tok, MemoCache &cache); // *%/
  std::shared_ptr<Term> parse_p1(TokenBuffer &tok, MemoCache &cache); // Unary prefix op
  std::shared_ptr<Term> parse_p0(TokenBuffer &tok, MemoCache &cache); // member access
  std::shared_ptr<Term> parse_expression(TokenBuffer &tok, MemoCache &cache);
  std::shared_ptr<Term> compile(QStringView expr, void *builtins = nullptr);

private:
  ExpressionCache &_cache;
  template <typename T> std::shared_ptr<T> accept(T &&v) { return _cache.add_or_return<T>(std::forward<T>(v)); }
  typedef std::shared_ptr<Term> (Parser::*ParseFn)(TokenBuffer &tok, MemoCache &cache);
  std::shared_ptr<Term> parse_binary_infix(TokenBuffer &tok, MemoCache &cache,
                                           const std::set<BinaryInfix::Operators> &valid, ParseFn lhs, ParseFn rhs);
  // Workaround to make parse_identifier into a ParseFn.
  std::shared_ptr<Term> parse_identifier_as_term(TokenBuffer &tok, MemoCache &cache);
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

std::shared_ptr<Expression> compile(std::string_view expr, ExpressionCache &cache, void *builtins = nullptr);
inline std::shared_ptr<Expression> optimize(Expression &expr, ExpressionCache &cache) { return nullptr; };
template <typename T> T evaluate(Expression &expr, Parameters &params) { return T(); };
inline std::string format(Expression &expr) { return ""; }

} // namespace pepp::debug
