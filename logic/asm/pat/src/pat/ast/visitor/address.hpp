#include <boost/variant/static_visitor.hpp>

#pragma once
namespace pat::ast::visitor {
struct AssignAddress : public boost::static_visitor<> {};
struct AdjustOffset : public boost::static_visitor<> {};
} // namespace pat::ast::visitor
