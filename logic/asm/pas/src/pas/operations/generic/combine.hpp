#pragma once
#include "pas/ast/node.hpp"
#include "pas/pas_globals.hpp"

namespace pas::ops::generic {
namespace detail {
struct PAS_EXPORT Traits {
  quint64 base, size, alignment;
};
Traits PAS_EXPORT getTraits(const ast::Node &section);

bool PAS_EXPORT isOrgSection(const ast::Node &section);
quint64 PAS_EXPORT minAddress(const ast::Node &section);
void PAS_EXPORT addOffset(ast::Node &section, qsizetype offset);
} // namespace detail
bool PAS_EXPORT concatSectionAddresses(ast::Node &root);
} // namespace pas::ops::generic
