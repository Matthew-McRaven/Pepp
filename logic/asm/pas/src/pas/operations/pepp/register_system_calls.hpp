#pragma once
#include "macro/registry.hpp"
#include "pas/ast/node.hpp"
#include "pas/ast/op.hpp"
#include "pas/pas_globals.hpp"

namespace pas::ops::pepp {
// Mus satisfy pas::ops::generic::isDirective
struct PAS_EXPORT RegisterSystemCalls : public pas::ops::MutatingOp<bool> {
  QSharedPointer<macro::Registry> registry;
  bool addedError = false;
  bool operator()(pas::ast::Node &node) override;
};

// Returns true if operation succeded.
bool PAS_EXPORT registerSystemCalls(pas::ast::Node &node,
                         QSharedPointer<macro::Registry> registry);
} // namespace pas::ops::pepp
