/*
 * Copyright (c) 2023 J. Stanley Warford, Matthew McRaven
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once
#include "asm/pas/ast/node.hpp"
#include "asm/pas/pas_globals.hpp"

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
