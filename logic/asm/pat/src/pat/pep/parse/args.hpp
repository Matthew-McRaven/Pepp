#pragma once

#include "../ast/values.hpp"
#include <QtCore>
#include <boost/spirit/home/x3.hpp>

namespace pat::pep::parse {
using boost::spirit::x3::char_;
using boost::spirit::x3::char_range;
using boost::spirit::x3::digit;
using boost::spirit::x3::eps;
using boost::spirit::x3::int_;
using boost::spirit::x3::lexeme;
using boost::spirit::x3::lit;
using boost::spirit::x3::no_skip;
using boost::spirit::x3::raw;
using boost::spirit::x3::rule;
using boost::spirit::x3::space;
using boost::spirit::x3::uint_parser;
// character / string components
const auto escape_codes = lit("\\b") | lit("\\f") | lit("\\n") | lit("\\r") |
                          lit("\\t") | lit("\\v") | lit("\\\"") | lit("\\'");
const auto hex_chars = char_("a", "f") | char_("A", "F") | char_("0", "9");
const auto escape_hex_code = lit("\\") >> lit("x") |
                             lit("X") >> hex_chars >> hex_chars;
const auto inner_char = (char_ - "\\") | escape_codes | escape_hex_code;

// Character Literal
rule<class character, pep::ast::CharacterLiteral> character = "character";
const auto character_def = lexeme["'" >> raw[-(inner_char - "'")] >> "'"];
BOOST_SPIRIT_DEFINE(character);

// String Literal
rule<class strings, pep::ast::StringLiteral> strings = "strings";
const auto strings_def = lexeme["\"" >> raw[*(inner_char - "\"")] >> "\""];
BOOST_SPIRIT_DEFINE(strings);

// Identifier
const auto ident_char =
    char_ - (space | "\"" | lit("'") | lit(":") | lit(";") | "," | ".");
rule<class identifier, pep::ast::Identifier> identifier = "identifier";
const auto identifier_def = lexeme[raw[(ident_char - digit) >> *ident_char]];
BOOST_SPIRIT_DEFINE(identifier);

// Decimal Literal
rule<class decimal, pep::ast::DecimalLiteral> decimal = "decimal";
const auto decimal_def = int_;
BOOST_SPIRIT_DEFINE(decimal);

// Hexadecimal Literal
rule<class hexadecimal, pep::ast::HexadecimalLiteral> hexadecimal =
    "hexadecimal";
const auto hexadecimal_def = "0x" >> no_skip[uint_parser<quint64, 16>{}];
BOOST_SPIRIT_DEFINE(hexadecimal);

// Argument Non-terminal
rule<class argument, pep::ast::Value> argument = "argument";
const auto argument_def =
    strings | character | identifier | hexadecimal | decimal;
BOOST_SPIRIT_DEFINE(argument);
} // namespace pat::pep::parse
