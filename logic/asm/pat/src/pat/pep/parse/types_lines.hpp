#pragma once
#include "./types_values.hpp"
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/home/x3.hpp>
#include <climits>
#include <string>
namespace pat::pep::parse {
struct BlankType {};

struct CommentType {
  std::string comment;
};

struct UnaryType {
  std::string symbol, identifier, comment;
  bool hasComment = false;
};

struct NonUnaryType {
  std::string symbol, identifier;
  parse::Value arg;
  std::string addr, comment;
  bool hasComment = false;
};

struct DirectiveType {
  std::string symbol, identifier;
  std::vector<parse::Value> args;
  std::string comment;
  bool hasComment = false;
};

struct MacroType {
  std::string symbol, identifier;
  std::vector<parse::Value> args;
  std::string comment;
  bool hasComment = false;
};

typedef boost::variant<BlankType, CommentType, UnaryType, NonUnaryType,
                       DirectiveType, MacroType>
    LineType;
} // namespace pat::pep::parse

BOOST_FUSION_ADAPT_STRUCT(pat::pep::parse::BlankType);
BOOST_FUSION_ADAPT_STRUCT(pat::pep::parse::CommentType, comment);
BOOST_FUSION_ADAPT_STRUCT(pat::pep::parse::UnaryType, symbol, identifier,
                          comment);
BOOST_FUSION_ADAPT_STRUCT(pat::pep::parse::NonUnaryType, symbol, identifier,
                          arg, addr, comment);
BOOST_FUSION_ADAPT_STRUCT(pat::pep::parse::DirectiveType, symbol, identifier,
                          args, comment);
BOOST_FUSION_ADAPT_STRUCT(pat::pep::parse::MacroType, symbol, identifier, args,
                          comment);
