#pragma once
#include <boost/mpl/vector.hpp>

namespace pat::ast::node {
class Blank;
class Comment;
class Directive;
class Error;
using BaseTypes = boost::mpl::vector<node::Directive *, node::Blank *,
                                     node::Comment *, node::Error *>;
} // namespace pat::ast::node
