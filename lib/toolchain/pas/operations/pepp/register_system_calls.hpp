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
#include "toolchain/pas/ast/node.hpp"
#include "toolchain/pas/ast/op.hpp"
#include "toolchain/macro/registry.hpp"

namespace pas::ops::pepp {
// Mus satisfy pas::ops::generic::isDirective
struct RegisterSystemCalls : public pas::ops::MutatingOp<bool> {
  QSharedPointer<macro::Registry> registry;
  bool addedError = false;
  bool operator()(pas::ast::Node &node) override;
};

// Returns true if operation succeded.
bool registerSystemCalls(pas::ast::Node &node, QSharedPointer<macro::Registry> registry);
} // namespace pas::ops::pepp
