/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
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
#include <functional>
#include <span>
#include <vector>
#include "core/formats/elf/managed_types.hpp"

namespace pepp::bts {

class ManagedSection;
class ManagedElf;

// Return the sections that are destined for serialization to an output file. Order is irrelevant.
// Sections for which the `keep` predicate returns true are kept, as are any sections reachable through their sh_link
// and sh_info fields (where applicable). Pseudo-sections are always stripped. Effectively a mark-sweep GC.
std::vector<SectionRef> garbage_collect_sections(const ManagedElf &elf,
                                                 const std::function<bool(const ManagedSection &)> &keep);
// Overload of above where keep predicate always returns true.
std::vector<SectionRef> garbage_collect_sections(const ManagedElf &elf);
} // namespace pepp::bts
