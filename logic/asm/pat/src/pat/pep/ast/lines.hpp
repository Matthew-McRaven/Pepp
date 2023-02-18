#pragma once
#include "./values.hpp"
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/home/x3.hpp>
#include <climits>
#include <string>
namespace pat::pep::ast {
struct Blank {};

struct Comment {
  std::string comment;
};

struct Unary {
  std::string symbol, identifier, comment;
};

struct NonUnary {
  std::string symbol, identifier;
  pep::ast::Value arg;
  std::string addr, comment;
};

struct Directive {
  std::string symbol, identifier;
  std::vector<ast::Value> args;
  std::string comment;
};

struct Macro {
  std::string symbol, identifier;
  std::vector<ast::Value> args;
  std::string comment;
};

typedef boost::variant<Blank, Comment, Unary, NonUnary, Directive, Macro> Line;
} // namespace pat::pep::ast

BOOST_FUSION_ADAPT_STRUCT(pat::pep::ast::Blank);
BOOST_FUSION_ADAPT_STRUCT(pat::pep::ast::Comment, comment);
BOOST_FUSION_ADAPT_STRUCT(pat::pep::ast::Unary, symbol, identifier, comment);
BOOST_FUSION_ADAPT_STRUCT(pat::pep::ast::NonUnary, symbol, identifier, arg,
                          addr, comment);
BOOST_FUSION_ADAPT_STRUCT(pat::pep::ast::Directive, symbol, identifier, args,
                          comment);
BOOST_FUSION_ADAPT_STRUCT(pat::pep::ast::Macro, symbol, identifier, args,
                          comment);
