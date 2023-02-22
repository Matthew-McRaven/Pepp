#pragma once
#include <boost/variant/static_visitor.hpp>
namespace pat::ast::visitors {
struct ToBytes : public boost::static_visitor<> {};
} // namespace pat::ast::visitors
