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

struct Parser {
  Parser(ExpressionCache &cache);
  inline std::shared_ptr<Register> parse_register() { return nullptr; }
  inline std::shared_ptr<Term> parse_value() { return nullptr; }
  std::shared_ptr<Term> parse_parened();
  std::shared_ptr<Constant> parse_constant();
  inline std::shared_ptr<Term> parse_p5() { return nullptr; }
  inline std::shared_ptr<Term> parse_p4() { return nullptr; }
  inline std::shared_ptr<Term> parse_p3() { return nullptr; }
  inline std::shared_ptr<Term> parse_p2() { return nullptr; }
  inline std::shared_ptr<Term> parse_p1() { return nullptr; }
  inline std::shared_ptr<Term> parse_p0() { return nullptr; }
  inline std::shared_ptr<Term> parse_expression() { return nullptr; }
  std::shared_ptr<Term> compile(QStringView expr, void *builtins = nullptr);
  ExpressionCache &_cache;
  Lexer _lex;
  std::list<Lexer::Token> _list = {};
  template <typename T> Lexer::Token match() {
    auto next = peek<T>();
    if (std::holds_alternative<T>(next)) _list.pop_front();
    return next;
  }
  template <typename T> Lexer::Token peek() {
    if (_list.empty()) _list.push_front(_lex.next_token());
    if (auto l = _list.front(); std::holds_alternative<T>(l)) return l;
    return std::monostate{};
  }
};

std::shared_ptr<Expression> compile(std::string_view expr, ExpressionCache &cache, void *builtins = nullptr);
inline std::shared_ptr<Expression> optimize(Expression &expr, ExpressionCache &cache) { return nullptr; };
template <typename T> T evaluate(Expression &expr, Parameters &params) { return T(); };
inline std::string format(Expression &expr) { return ""; }

} // namespace pepp::debug
