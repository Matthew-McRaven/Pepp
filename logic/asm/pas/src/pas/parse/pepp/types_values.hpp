#pragma once
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/home/x3.hpp>
#include <climits>
#include <string>
namespace pas::parse::pepp {
struct TextLiteral {
  std::string value;
};
struct CharacterLiteral : public TextLiteral {};

struct StringLiteral : public TextLiteral {};
struct Identifier : public TextLiteral {};

struct NumericLiteral {
  uint64_t value;
};
struct DecimalLiteral : public NumericLiteral {
  bool isSigned = false;
};
struct HexadecimalLiteral : public NumericLiteral {};

typedef boost::variant<StringLiteral, CharacterLiteral, Identifier,
                       DecimalLiteral, HexadecimalLiteral>
    Value;
using boost::fusion::operator<<;
} // namespace pas::parse::pepp

BOOST_FUSION_ADAPT_STRUCT(pas::parse::pepp::CharacterLiteral, value);
BOOST_FUSION_ADAPT_STRUCT(pas::parse::pepp::StringLiteral, value);
BOOST_FUSION_ADAPT_STRUCT(pas::parse::pepp::Identifier, value);

BOOST_FUSION_ADAPT_STRUCT(pas::parse::pepp::DecimalLiteral, value);
BOOST_FUSION_ADAPT_STRUCT(pas::parse::pepp::HexadecimalLiteral, value);
