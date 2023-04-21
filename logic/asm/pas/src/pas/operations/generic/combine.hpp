#pragma once
#include "pas/ast/node.hpp"
namespace pas::ops::generic {
namespace detail {
struct Traits {
  quint64 base, size, alignment;
};
Traits getTraits(const ast::Node &section);

bool isOrgSection(const ast::Node &section);
quint64 minAddress(const ast::Node &section);
void addOffset(ast::Node &section, qsizetype offset);
} // namespace detail
bool concatSectionAddresses(ast::Node &root);
} // namespace pas::ops::generic
