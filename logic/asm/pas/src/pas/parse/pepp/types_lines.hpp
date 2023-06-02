#pragma once
#include "./types_values.hpp"
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/home/x3.hpp>
#include <climits>
#include <string>
#include "pas/pas_globals.hpp"

namespace pas::parse::pepp {
struct PAS_EXPORT BlankType {};

struct PAS_EXPORT CommentType {
  std::string comment;
};

struct PAS_EXPORT UnaryType {
  std::string symbol, identifier, comment;
  bool hasComment = false;
};

struct PAS_EXPORT NonUnaryType {
  std::string symbol, identifier;
  Value arg;
  std::string addr, comment;
  bool hasComment = false;
};

struct PAS_EXPORT DirectiveType {
  std::string symbol, identifier;
  std::vector<Value> args;
  std::string comment;
  bool hasComment = false;
};

struct PAS_EXPORT MacroType {
  std::string symbol, identifier;
  std::vector<Value> args;
  std::string comment;
  bool hasComment = false;
};

typedef boost::variant<BlankType, CommentType, UnaryType, NonUnaryType,
                       DirectiveType, MacroType>
    LineType;
} // namespace pas::parse::pepp

BOOST_FUSION_ADAPT_STRUCT(pas::parse::pepp::BlankType);
BOOST_FUSION_ADAPT_STRUCT(pas::parse::pepp::CommentType, comment);
BOOST_FUSION_ADAPT_STRUCT(pas::parse::pepp::UnaryType, symbol, identifier,
                          comment);
BOOST_FUSION_ADAPT_STRUCT(pas::parse::pepp::NonUnaryType, symbol, identifier,
                          arg, addr, comment);
BOOST_FUSION_ADAPT_STRUCT(pas::parse::pepp::DirectiveType, symbol, identifier,
                          args, comment);
BOOST_FUSION_ADAPT_STRUCT(pas::parse::pepp::MacroType, symbol, identifier, args,
                          comment);
