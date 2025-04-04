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
      _set.insert(ret);
      return ret;
    } else return std::dynamic_pointer_cast<T>(*search);
  }
};

struct TokenBuffer {
  inline TokenBuffer(QStringView expr) : _lex(Lexer(expr)), _tokens(), _head(0) {}
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

struct TokenCheckpoint {
  inline TokenCheckpoint(TokenBuffer &buf) : _head(buf._head), _buf(buf) {}
  template <typename T> inline std::shared_ptr<T> rollback() {
    _buf._head = _head;
    return nullptr;
  }
  size_t _head = 0;
  TokenBuffer &_buf;
};

struct Parser {
  Parser(ExpressionCache &cache);
  std::shared_ptr<Register> parse_register(TokenBuffer &tok);
  std::shared_ptr<Term> parse_value(TokenBuffer &tok);
  std::shared_ptr<Term> parse_parened(TokenBuffer &tok);
  std::shared_ptr<Constant> parse_constant(TokenBuffer &tok);
  std::shared_ptr<Variable> parse_identifier(TokenBuffer &tok);
  std::shared_ptr<Term> parse_p5(TokenBuffer &tok);
  std::shared_ptr<Term> parse_p4(TokenBuffer &tok);
  std::shared_ptr<Term> parse_p3(TokenBuffer &tok);
  std::shared_ptr<Term> parse_p2(TokenBuffer &tok);
  std::shared_ptr<Term> parse_p1(TokenBuffer &tok);
  std::shared_ptr<Term> parse_p0(TokenBuffer &tok);
  std::shared_ptr<Term> parse_expression(TokenBuffer &tok);
  std::shared_ptr<Term> compile(QStringView expr, void *builtins = nullptr);

private:
  ExpressionCache &_cache;
  template <typename T> std::shared_ptr<T> accept(T &&v) { return _cache.add_or_return<T>(std::move(v)); }
  typedef std::shared_ptr<Term> (Parser::*ParseFn)(TokenBuffer &tok);
  std::shared_ptr<Term> parse_binary_infix(TokenBuffer &tok, const std::set<BinaryInfix::Operators> &valid,
                                           ParseFn parse);
  // Workaround to make parse_identifier into a ParseFn.
  std::shared_ptr<Term> parse_identifier_as_term(TokenBuffer &tok);
};

std::shared_ptr<Expression> compile(std::string_view expr, ExpressionCache &cache, void *builtins = nullptr);
inline std::shared_ptr<Expression> optimize(Expression &expr, ExpressionCache &cache) { return nullptr; };
template <typename T> T evaluate(Expression &expr, Parameters &params) { return T(); };
inline std::string format(Expression &expr) { return ""; }

} // namespace pepp::debug
