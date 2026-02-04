/*
 * Copyright (c) 2023-2026 J. Stanley Warford, Matthew McRaven
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

#include "core/compile/symbol/entry.hpp"
#include "core/compile/symbol/types.hpp"
#include "core/compile/symbol/value.hpp"

pepp::core::symbol::Entry::Entry(symbol::LeafTable &parent, std::string_view name) noexcept
    : parent(parent), name(name), state(DefinitionState::Undefined), binding(Binding::Local),
      value(std::make_shared<symbol::EmptyValue>(0)) {}

bool pepp::core::symbol::Entry::is_singly_defined() const noexcept { return state == DefinitionState::Single; }

bool pepp::core::symbol::Entry::is_undefined() const noexcept { return state == DefinitionState::Undefined; }
