#include "expr_parser.hpp"

#include "expr_ast.hpp"

pepp::debug::Parser::Parser(ExpressionCache &cache) : _cache(cache), _lex({}) {}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::parse_parened() {
  using Lit = detail::Literal;
  if (auto maybe_open = match<Lit>(); !std::holds_alternative<Lit>(maybe_open)) {
    return nullptr;
  } else if (auto open = std::get<Lit>(maybe_open); open.literal != '(') {
    _list.push_front(maybe_open);
    return nullptr;
  } else if (auto inner = parse_expression(); !inner) {
    return nullptr;
  } else if (auto maybe_close = match<Lit>(); !std::holds_alternative<Lit>(maybe_close)) {
    return nullptr;
  } else if (auto close = std::get<Lit>(maybe_close); close.literal != ')') {
    // Invalid and we cannot recover.
    return nullptr;
  } else {
    return _cache.add_or_return<Parenthesized>(Parenthesized(inner));
  }
}

std::shared_ptr<pepp::debug::Constant> pepp::debug::Parser::parse_constant() {
  using UC = detail::UnsignedConstant;
  if (auto maybe_constant = match<UC>(); !std::holds_alternative<UC>(maybe_constant)) {
    return nullptr;
  } else {
    return _cache.add_or_return<Constant>(Constant(std::get<UC>(maybe_constant)));
  }
}

std::shared_ptr<pepp::debug::Term> pepp::debug::Parser::compile(QStringView expr, void *builtins) {
  _lex = Lexer(expr);
  _list.clear();
  return parse_constant();
}

std::shared_ptr<pepp::debug::Expression> pepp::debug::compile(std::string_view expr, ExpressionCache &cache,
                                                              void *builtins) {
  return nullptr;
}
