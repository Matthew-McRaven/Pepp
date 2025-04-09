#include "expr_tokenizer.hpp"

namespace re {
static const QRegularExpression whitespace("[ \r\t\n]+");
static const QRegularExpression hex("0[xX][a-fA-F0-9]+");
static const QRegularExpression dec("[+-]?[0-9]+");
static const QRegularExpression constant_type_spec("_[iu](8|16|32)");
static const QRegularExpression debug(R"(\$\w+)");
static const QRegularExpression ident(R"(\w+)");
// Longest sequences must be first, because QRegularExpression stops at first match, not longest.
static const QRegularExpression literal("==|<=|>=|!=|->|<<|>>|[=<>()/!~%^&*\\-+\\.|]");
using T = pepp::debug::ExpressionType;
const std::map<QString, T> types = {
    {"i8", T::i8}, {"u8", T::u8}, {"i16", T::i16}, {"u16", T::u16}, {"i32", T::i32}, {"u32", T::u32},
};
} // namespace re

pepp::debug::Lexer::Lexer(QStringView input) : _input(input), _offset(0) {}

pepp::debug::Lexer::Token pepp::debug::Lexer::next_token() {
  using namespace pepp::debug::detail;
  static const auto mt = QRegularExpression::MatchType::NormalMatch;
  static const auto opt = QRegularExpression::MatchOption::AnchorAtOffsetMatchOption;
  if (_offset == _input.length()) return detail::T<TokenType::Eof>{};
  else if (auto trailing_type = re::constant_type_spec.matchView(_input, _offset, mt, opt);
           _allows_trailing_type && trailing_type.hasMatch()) {
    _allows_trailing_type = false;
    _offset = trailing_type.capturedEnd();
    auto type_str = trailing_type.capturedView().slice(1); // drop leading _
    using T = detail::T<TokenType::ConstantType>;
    return T{.type = re::types.at(type_str.toString())};
  } else _allows_trailing_type = false;

  if (auto space_match = re::whitespace.matchView(_input, _offset, mt, opt); space_match.hasMatch()) {
    _offset = space_match.capturedEnd();
  }
  if (auto hex_match = re::hex.matchView(_input, _offset, mt, opt); hex_match.hasMatch()) {
    _offset = hex_match.capturedEnd();
    _allows_trailing_type = true;
    using T = detail::T<TokenType::UnsignedConstant>;
    return T{.format = T::Format::Hex,
             .value = static_cast<uint64_t>(hex_match.capturedView().slice(2).toInt(nullptr, 16))};
  } else if (auto dec_match = re::dec.matchView(_input, _offset, mt, opt); dec_match.hasMatch()) {
    _offset = dec_match.capturedEnd();
    _allows_trailing_type = true;
    using T = detail::T<TokenType::UnsignedConstant>;
    return T{.format = T::Format::Dec, .value = static_cast<uint64_t>(dec_match.capturedView().toInt(nullptr, 10))};
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

std::strong_ordering pepp::debug::detail::T<pepp::debug::detail::TokenType::UnsignedConstant>::operator<=>(
    const T<TokenType::UnsignedConstant> &rhs) const {
  if (auto cmp = format <=> rhs.format; cmp != 0) return cmp;
  return value <=> rhs.value;
}
