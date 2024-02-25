/*
 * Copyright (c) 2023-2024 J. Stanley Warford, Matthew McRaven
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
#include <QRegularExpression>
#include <QTextCharFormat>
#include "style/types.hpp"

namespace highlight {
struct Rule {
  QRegularExpression pattern;
  highlight::style::Types style;
  // If previousBlockState is fromState, execute the rule, and set the state to toState.
  // Can be used to implemente a FSM which handles block comments.
  int fromState = 0, toState = 0;
  // For this rule, search from index==0 rather than last index.
  bool reset = false;
};
} // namespace highlight
