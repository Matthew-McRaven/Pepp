#include <boost/variant/static_visitor.hpp>

#pragma once
namespace pat::ast::visitor {
struct ToSource : public boost::static_visitor<> {};
struct ToListing : public boost::static_visitor<> {};
struct ToHexcode : public boost::static_visitor<> {};
struct ToBytes : public boost::static_visitor<> {};
} // namespace pat::ast::visitor
