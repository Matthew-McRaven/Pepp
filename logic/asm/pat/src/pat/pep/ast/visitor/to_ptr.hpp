#include <boost/variant/static_visitor.hpp>

#include "pat/ast/node/base.hpp"

#pragma once
#pragma once
namespace pat::pep::ast::visitor {
struct ToPtr
    : public boost::static_visitor<QSharedPointer<pat::ast::node::Base>> {
  template <typename T> QSharedPointer<pat::ast::node::Base> operator()(T &t) {
    return t;
  };
};

} // namespace pat::pep::ast::visitor
