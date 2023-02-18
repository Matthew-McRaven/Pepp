#pragma once
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/home/x3.hpp>
#include <climits>
#include <string>
namespace pat::pep::ast {
struct TextLiteral {
  std::string value;
};
struct CharacterLiteral : public TextLiteral {};

struct StringLiteral : public TextLiteral {};
struct Identifier : public TextLiteral {};

struct NumericLiteral {
  int64_t value;
};
struct DecimalLiteral : public NumericLiteral {};
struct HexadecimalLiteral : public NumericLiteral {};

typedef boost::variant<StringLiteral, CharacterLiteral, Identifier,
                       DecimalLiteral, HexadecimalLiteral>
    Value;
using boost::fusion::operator<<;
} // namespace pat::pep::ast

BOOST_FUSION_ADAPT_STRUCT(pat::pep::ast::CharacterLiteral, value);
BOOST_FUSION_ADAPT_STRUCT(pat::pep::ast::StringLiteral, value);
BOOST_FUSION_ADAPT_STRUCT(pat::pep::ast::Identifier, value);

BOOST_FUSION_ADAPT_STRUCT(pat::pep::ast::DecimalLiteral, value);
BOOST_FUSION_ADAPT_STRUCT(pat::pep::ast::HexadecimalLiteral, value);
