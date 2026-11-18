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
#include <optional>
#include <span>
#include "core/formats/elf/enums_eheader.hpp"
#include "core/integers.h"
#include "core/link/diagnostics.hpp"

namespace pepp::link {

// An input file's relocation, with its symbol replaced by the output definition's address to which it resolved.
struct ResolvedRelocation {
  u64 offset = 0; // Of the relocated field, within its section.
  u32 type = 0;
  std::optional<u64> symbol; // S. Empty if the definition is not placed in the output.
  i64 addend = 0;            // A
};

// Apply a relocation to the bytes of its section. Returns the reason if it cannot be applied.
std::optional<RelocationCode> apply_relocation(bts::ElfMachineType machine, bts::ElfEndian endian,
                                               const ResolvedRelocation &rel, std::span<u8> section);
} // namespace pepp::link
