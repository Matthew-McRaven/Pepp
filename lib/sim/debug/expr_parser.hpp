#pragma once
#include <QtCore>
#include <set>
#include "expr_ast.hpp"
#include "expr_cache.hpp"
#include "expr_types.hpp"

namespace pepp::debug {

using ExpressionCache = Cache<Term>;

namespace detail {
struct TokenBuffer;
struct MemoCache;
} // namespace detail

struct Parser {
  // Helper to associate regions with parsing rules/functions in Memo/MemoCache
  enum class Rule : uint8_t {
    DEBUG_IDENT,
    VALUE,
    PAREN,
    CONSTANT,
    IDENT,
    TYPECAST,
    P0,
    P1,
    P2,
    P3,
    P4,
    P5,
    P6,
    P7,
    INVALID
  };

  explicit Parser(ExpressionCache &cache, types::TypeCache *types = nullptr);
  std::shared_ptr<Term> compile(QStringView expr, void *builtins = nullptr);
  std::shared_ptr<Term> compile(QString expr, void *builtins = nullptr);
  std::shared_ptr<types::Type> compile_type(QStringView expr, void *builtins = nullptr);
  std::shared_ptr<types::Type> compile_type(QString expr, void *builtins = nullptr);

private:
  ExpressionCache &_cache;
  types::TypeCache *_types = nullptr;
  template <typename T> std::shared_ptr<T> accept(T &&v) { return _cache.add_or_return<T>(std::forward<T>(v)); }
  std::shared_ptr<DebuggerVariable> parse_debug_identifier(detail::TokenBuffer &tok, detail::MemoCache &cache);
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

  using ParseFn = std::shared_ptr<Term> (Parser::*)(detail::TokenBuffer &tok, detail::MemoCache &cache);
  std::shared_ptr<Term> parse_binary_infix(detail::TokenBuffer &tok, detail::MemoCache &cache,
                                           const std::set<BinaryInfix::Operators> &valid, ParseFn lhs, ParseFn rhs);
};

} // namespace pepp::debug
