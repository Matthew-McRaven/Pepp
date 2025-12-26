/*
 * Copyright (c) 2024-2026 J. Stanley Warford, Matthew McRaven
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
#include <QtCore>
#include "enums/constants.hpp"

namespace builtins {
class Registry;
}
class HelpEntry;
QSharedPointer<HelpEntry> starting_root();
QSharedPointer<HelpEntry> ui_root();
QSharedPointer<HelpEntry> workflows_root();
QSharedPointer<HelpEntry> advanced_root();
QSharedPointer<HelpEntry> greencard10_root();
QSharedPointer<HelpEntry> greencard9_root();
std::array<QSharedPointer<HelpEntry>, 3> examples_root(const builtins::Registry &reg);
QSharedPointer<HelpEntry> problems_root(const builtins::Registry &reg);
QSharedPointer<HelpEntry> os_root();
QSharedPointer<HelpEntry> macros_root(const builtins::Registry &reg);
int bitmask(pepp::Architecture arch);
int bitmask(pepp::Abstraction level);
int bitmask(pepp::Architecture arch, pepp::Abstraction level);
int bitmask_all_levels(pepp::Architecture arch);
bool masked(int lhs, int rhs);
