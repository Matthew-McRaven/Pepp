#pragma once

#include "./rules_values.hpp"
#include "./types_lines.hpp"
#include "./types_values.hpp"
#include <QtCore>
#include <boost/spirit/home/x3.hpp>
namespace pat::pep::parse {
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
rule<class blank, parse::BlankType> blank = "blank";
const auto blank_def = eps;
BOOST_SPIRIT_DEFINE(blank);

// Comment Line
rule<class comment, parse::CommentType> comment = "comment";
const auto comment_def = lexeme[lit(";") >> *(char_)];
BOOST_SPIRIT_DEFINE(comment);

const auto symbol = parse::identifier_def >> lit(":");
const auto setComment = [](auto &ctx) { _val(ctx).hasComment = true; };
// Unary Line
// 3rd template param = true needed to force auto attribute propogation with
// semantic actions
rule<class unary, parse::UnaryType, true> unary = "unary";
const auto unary_def =
    skip(space)[-symbol >> identifier_def >> -comment_def[setComment]];
BOOST_SPIRIT_DEFINE(unary);

// NonUnary Line
// 3rd template param = true needed to force auto attribute propogation with
// semantic actions.
rule<class nonunary, parse::NonUnaryType, true> nonunary = "nonunary";
const auto nonunary_def =
    skip(space)[-symbol >> identifier_def >> parse::argument >>
                -("," >> identifier_def) >> -comment_def[setComment]];
BOOST_SPIRIT_DEFINE(nonunary);

// Directive Line
// 3rd template param = true needed to force auto attribute propogation with
// semantic actions.
rule<class directive, parse::DirectiveType, true> directive = "directive";
const auto directive_def =
    skip(space)[-symbol >> lexeme["." >> identifier_def] >>
                *(parse::argument % ",") >> -comment_def[setComment]];
BOOST_SPIRIT_DEFINE(directive);

// Directive Line
// 3rd template param = true needed to force auto attribute propogation with
// semantic actions.
rule<class macro, parse::MacroType, true> macro = "macro";
const auto macro_def =
    skip(space)[-symbol >> lexeme["@" >> identifier_def] >>
                *(parse::argument % ",") >> -comment_def[setComment]];
BOOST_SPIRIT_DEFINE(macro);

// Lines
rule<class line, std::vector<parse::LineType>> line = "line";
const auto line_def =
    (directive | macro | unary | nonunary | comment | blank) % eol;
BOOST_SPIRIT_DEFINE(line);

} // namespace pat::pep::parse
