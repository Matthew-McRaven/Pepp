#pragma once
#include "pat/ast/node/types.hpp"
#include <boost/mpl/vector.hpp>
#include <boost/variant/variant.hpp>
namespace pat::pep::ast::node {
template <typename ISA> class Instruction;

template <typename ISA>
using PepTypes = typename boost::mpl::push_front<pat::ast::node::BaseTypes,
                                                 Instruction<ISA> *>::type;
template <typename ISA>
using Node = typename boost::make_variant_over<PepTypes<ISA>>::type;
} // namespace pat::pep::ast::node
