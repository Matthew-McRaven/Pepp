#pragma once
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/home/x3.hpp>
#include <climits>
#include <string>
namespace pat::pep::parse {
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
} // namespace pat::pep::parse

BOOST_FUSION_ADAPT_STRUCT(pat::pep::parse::CharacterLiteral, value);
BOOST_FUSION_ADAPT_STRUCT(pat::pep::parse::StringLiteral, value);
BOOST_FUSION_ADAPT_STRUCT(pat::pep::parse::Identifier, value);

BOOST_FUSION_ADAPT_STRUCT(pat::pep::parse::DecimalLiteral, value);
BOOST_FUSION_ADAPT_STRUCT(pat::pep::parse::HexadecimalLiteral, value);
