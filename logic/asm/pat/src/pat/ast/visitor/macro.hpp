#include <boost/variant/static_visitor.hpp>

#pragma once
namespace pat::ast::visitor {
  struct ExpandMacros : public boost::static_visitor<>{};
}
