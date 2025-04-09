#pragma once
#include <QtCore>
#include <set>
#include "expr_ast.hpp"

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

namespace detail {
struct TokenBuffer;
struct MemoCache;
} // namespace detail

struct Parser {
  // Helper to associate regions with parsing rules/functions in Memo/MemoCache
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
  std::shared_ptr<Term> compile(QStringView expr, void *builtins = nullptr);

private:
  ExpressionCache &_cache;
  template <typename T> std::shared_ptr<T> accept(T &&v) { return _cache.add_or_return<T>(std::forward<T>(v)); }
  std::shared_ptr<Register> parse_register(detail::TokenBuffer &tok, detail::MemoCache &cache);
  std::shared_ptr<Term> parse_value(detail::TokenBuffer &tok, detail::MemoCache &cache);
  std::shared_ptr<Term> parse_parened(detail::TokenBuffer &tok, detail::MemoCache &cache);
  std::shared_ptr<Constant> parse_constant(detail::TokenBuffer &tok, detail::MemoCache &cache);
  std::shared_ptr<Variable> parse_identifier(detail::TokenBuffer &tok, detail::MemoCache &cache);
  // Workaround to make parse_identifier into a ParseFn.
  std::shared_ptr<Term> parse_identifier_as_term(detail::TokenBuffer &tok, detail::MemoCache &cache);
  std::shared_ptr<Term> parse_p7(detail::TokenBuffer &tok, detail::MemoCache &cache); // Bitwise ops
  std::shared_ptr<Term> parse_p6(detail::TokenBuffer &tok, detail::MemoCache &cache); // Equality
  std::shared_ptr<Term> parse_p5(detail::TokenBuffer &tok, detail::MemoCache &cache); // Inequality
  std::shared_ptr<Term> parse_p4(detail::TokenBuffer &tok, detail::MemoCache &cache); // Bitwise shifts
  std::shared_ptr<Term> parse_p3(detail::TokenBuffer &tok, detail::MemoCache &cache); // +-
  std::shared_ptr<Term> parse_p2(detail::TokenBuffer &tok, detail::MemoCache &cache); // *%/
  std::shared_ptr<Term> parse_p1(detail::TokenBuffer &tok, detail::MemoCache &cache); // Unary prefix op
  std::shared_ptr<Term> parse_p0(detail::TokenBuffer &tok, detail::MemoCache &cache); // member access
  std::shared_ptr<Term> parse_expression(detail::TokenBuffer &tok, detail::MemoCache &cache);

  using ParseFn = std::shared_ptr<Term> (Parser::*)(detail::TokenBuffer &tok, detail::MemoCache &cache);
  std::shared_ptr<Term> parse_binary_infix(detail::TokenBuffer &tok, detail::MemoCache &cache,
                                           const std::set<BinaryInfix::Operators> &valid, ParseFn lhs, ParseFn rhs);
};

} // namespace pepp::debug
