#pragma once

#include "./rules_values.hpp"
#include "./types_lines.hpp"
#include "./types_values.hpp"
#include <QtCore>
#include <boost/spirit/home/x3.hpp>
namespace pas::parse::pepp {
using boost::spirit::x3::attr;
using boost::spirit::x3::char_;
using boost::spirit::x3::char_range;
using boost::spirit::x3::digit;
using boost::spirit::x3::eol;
using boost::spirit::x3::eps;
using boost::spirit::x3::int_;
using boost::spirit::x3::lexeme;
using boost::spirit::x3::lit;
using boost::spirit::x3::no_skip;
using boost::spirit::x3::raw;
using boost::spirit::x3::rule;
using boost::spirit::x3::skip;
using boost::spirit::x3::space;
using boost::spirit::x3::uint_parser;

// Blank Line
inline rule<class blank, BlankType> blank = "blank";
inline const auto blank_def = skip(space - eol)[eps];
BOOST_SPIRIT_DEFINE(blank);

// Comment Line
inline rule<class comment, CommentType> comment = "comment";
inline const auto comment_def = skip(space - eol)[lexeme[lit(";") >> *(char_ - eol)]];
BOOST_SPIRIT_DEFINE(comment);

inline const auto symbol = identifier_def >> ":";
inline const auto setComment = [](auto &ctx) { _val(ctx).hasComment = true; };
// Unary Line
// 3rd template param = true needed to force auto attribute propogation with
// semantic actions
inline rule<class unary, UnaryType, true> unary = "unary";
// -thing doesn't work, so fake it by injecting an empty string into struct.
inline const auto unary_def =
    skip(space - eol)[(symbol | attr(std::string{})) >> identifier_def >>
                      (comment_def[setComment] | attr(std::string{}))];
BOOST_SPIRIT_DEFINE(unary);

// NonUnary Line
// 3rd template param = true needed to force auto attribute propogation with
// semantic actions.
inline rule<class nonunary, NonUnaryType, true> nonunary = "nonunary";
// -thing doesn't work, so fake it by injecting an empty string into struct.
inline const auto nonunary_def = skip(
    space - eol)[(symbol | attr(std::string{})) >> identifier_def >> argument >>
                 (("," >> identifier_def) | (attr(std::string{}))) >>
                 (comment_def[setComment] | attr(std::string{}))];
BOOST_SPIRIT_DEFINE(nonunary);

// Directive Line
// 3rd template param = true needed to force auto attribute propogation with
// semantic actions.
inline rule<class directive, DirectiveType, true> directive = "directive";
// -thing doesn't work, so fake it by injecting an empty string into struct.
inline const auto directive_def =
    skip(space -
         eol)[(symbol | attr(std::string{})) >> lexeme["." >> identifier_def] >>
              ((argument % ",") | attr(std::vector<Value>{})) >>
              (comment_def[setComment] | attr(std::string{}))];
BOOST_SPIRIT_DEFINE(directive);

// Directive Line
// 3rd template param = true needed to force auto attribute propogation with
// semantic actions.
inline rule<class macro, MacroType, true> macro = "macro";
// -thing doesn't work, so fake it by injecting an empty string into struct.
inline const auto macro_def =
    skip(space -
         eol)[(symbol | attr(std::string{})) >> lexeme["@" >> identifier_def] >>
              ((argument % ",") | attr(std::vector<Value>{})) >>
              (comment_def[setComment] | attr(std::string{}))];
BOOST_SPIRIT_DEFINE(macro);

// Lines
inline rule<class line, std::vector<LineType>> line = "line";
inline const auto line_def =
    (directive | macro | nonunary | unary | comment | blank) % eol;
BOOST_SPIRIT_DEFINE(line);

} // namespace pas::parse::pepp
