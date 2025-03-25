#include "expr_tokenizer.hpp"

namespace re {
static const QRegularExpression whitespace("[ \r\t\n]+");
static const QRegularExpression hex("0[xX][a-fA-F0-9]");
static const QRegularExpression dec("[+-]?[0-9]+");
static const QRegularExpression debug(R"(\$\w+)");
static const QRegularExpression ident(R"(\w+)");
static const QRegularExpression literal("[()/<>=!~%^&*_+]|==|<=|>=|!=|->");
} // namespace re

pepp::debug::Lexer::Lexer(QStringView input) : _input(input), _offset(0) {}

pepp::debug::Lexer::Token pepp::debug::Lexer::next_token() {
  using namespace pepp::debug::detail;
  static const auto mt = QRegularExpression::MatchType::NormalMatch;
  static const auto opt = QRegularExpression::MatchOption::AnchorAtOffsetMatchOption;
  if (_offset == _input.length()) return detail::T<TokenType::Invalid>{};
  if (auto space_match = re::whitespace.matchView(_input, _offset, mt, opt); space_match.hasMatch()) {
    _offset = space_match.capturedEnd();
  }
  if (auto hex_match = re::hex.matchView(_input, _offset, mt, opt); hex_match.hasMatch()) {
    _offset = hex_match.capturedEnd();
    using T = detail::T<TokenType::UnsignedConstant>;
    return T{.format = T::Format::Hex,
             .value = static_cast<uint64_t>(hex_match.capturedView().slice(2).toInt(nullptr, 16))};
  } else if (auto dec_match = re::dec.matchView(_input, _offset, mt, opt); dec_match.hasMatch()) {
    _offset = dec_match.capturedEnd();
    using T = detail::T<TokenType::UnsignedConstant>;
    return T{.format = T::Format::Dec, .value = static_cast<uint64_t>(hex_match.capturedView().toInt(nullptr, 10))};
  } else if (auto lit_match = re::literal.matchView(_input, _offset, mt, opt); lit_match.hasMatch()) {
    _offset = lit_match.capturedEnd();
    return detail::T<TokenType::Literal>{.literal = lit_match.captured()};
  } else if (auto debug_match = re::debug.matchView(_input, _offset, mt, opt); debug_match.hasMatch()) {
    _offset = debug_match.capturedEnd();
    return detail::T<TokenType::DebugIdentifier>{.value = debug_match.capturedView().slice(1).toString()};
  } else if (auto ident_match = re::ident.matchView(_input, _offset, mt, opt); ident_match.hasMatch()) {
    _offset = ident_match.capturedEnd();
    return detail::T<TokenType::Identifier>{.value = ident_match.capturedView().toString()};
  }
  return detail::T<TokenType::Invalid>();
}
