#pragma once
#include "pat/ast/node/types.hpp"
#include <QtCore>
#include <boost/mpl/vector.hpp>
#include <boost/variant/variant.hpp>
namespace pat::pep::ast::node {
template <typename ISA> class Instruction;

template <typename ISA>
using ChildTypes =
    typename boost::mpl::push_front<pat::ast::node::BaseTypes,
                                    QSharedPointer<Instruction<ISA>>>::type;
template <typename ISA> using NodeTypes = ChildTypes<ISA>;
template <typename ISA>
using ChildNode = typename boost::make_variant_over<ChildTypes<ISA>>::type;
} // namespace pat::pep::ast::node
