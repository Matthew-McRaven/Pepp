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
#include <vector>
#include "core/formats/elf/managed_types.hpp"

namespace pepp::bts {

class ManagedSection;
class ManagedElf;

// Create (or update) the .shstrtab section, filling it with the names of all live sections.
// Live list is mutable, because the .shstrab section must be added if not present.
SectionRef build_shstrtab(ManagedElf &elf, std::vector<SectionRef> &live);
} // namespace pepp::bts
