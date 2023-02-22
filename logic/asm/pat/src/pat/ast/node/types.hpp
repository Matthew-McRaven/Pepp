#pragma once
#include <QtCore>
#include <boost/mpl/vector.hpp>

namespace pat::ast::node {
class Blank;
class Comment;
class Directive;
class Error;
using BaseTypes = boost::mpl::vector<
    QSharedPointer<node::Directive>, QSharedPointer<node::Blank>,
    QSharedPointer<node::Comment>, QSharedPointer<node::Error>>;
} // namespace pat::ast::node
