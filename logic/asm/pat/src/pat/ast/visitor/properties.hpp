#include <boost/variant/static_visitor.hpp>

#pragma once
namespace pat::ast::visitors {
struct AdjustEmit : public boost::static_visitor<> {};
} // namespace pat::ast::visitors
