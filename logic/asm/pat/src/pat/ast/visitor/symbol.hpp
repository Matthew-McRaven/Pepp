#include <boost/variant/static_visitor.hpp>

#pragma once
namespace pat::ast::visitor {
struct ExtractSymbols : public boost::static_visitor<> {};
struct RegisterExports : public boost::static_visitor<> {};
} // namespace pat::ast::visitor
