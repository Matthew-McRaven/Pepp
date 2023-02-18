#pragma once

#include "../ast/lines.hpp"
#include "../ast/values.hpp"
#include "./args.hpp"
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
rule<class blank, pep::ast::Blank> blank = "blank";
const auto blank_def = eps;
BOOST_SPIRIT_DEFINE(blank);

// Comment Line
rule<class comment, pep::ast::Comment> comment = "comment";
const auto comment_def = lexeme[lit(";") >> *(char_)];
BOOST_SPIRIT_DEFINE(comment);

const auto symbol = parse::identifier_def >> lit(":");

// Unary Line
rule<class unary, pep::ast::Unary> unary = "unary";
const auto unary_def = skip(space)[-symbol >> identifier_def >> -comment_def];
BOOST_SPIRIT_DEFINE(unary);

// NonUnary Line
rule<class nonunary, pep::ast::NonUnary> nonunary = "nonunary";
const auto nonunary_def =
    skip(space)[-symbol >> identifier_def >> parse::argument >>
                -("," >> identifier_def) >> -comment_def];
BOOST_SPIRIT_DEFINE(nonunary);

// Directive Line
rule<class directive, pep::ast::Directive> directive = "directive";
const auto directive_def =
    skip(space)[-symbol >> lexeme["." >> identifier_def] >>
                *(parse::argument % ",") >> -comment_def];
BOOST_SPIRIT_DEFINE(directive);

// Directive Line
rule<class macro, pep::ast::Macro> macro = "macro";
const auto macro_def = skip(space)[-symbol >> lexeme["@" >> identifier_def] >>
                                   *(parse::argument % ",") >> -comment_def];
BOOST_SPIRIT_DEFINE(macro);

// Lines
rule<class line, pep::ast::Line> line = "line";
const auto line_def =
    (directive | macro | unary | nonunary | comment | blank) % eol;
BOOST_SPIRIT_DEFINE(line);

} // namespace pat::pep::parse
