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
#include <vector>
#include "core/formats/elf/packed_input_group.hpp"
#include "core/link/diagnostics.hpp"
#include "core/link/link_config.hpp"
#include "core/link/symbol_resolution.hpp"

namespace pepp::link {
// Create an empty group type matching the first input ELF file (including archive members).
// Returns nullopt if no input is an ELF file.
std::optional<bts::AnyElfGroup> detect_group_type(const std::vector<LinkInput> &inputs);
// Add every input to group, and set the liveness of each file it adds, indexed by file ID. Returns false after a Fatal
// error.
bool populate_group(Diagnostics &diagnostics, bts::AnyElfGroup &group, std::vector<LinkInput> inputs,
                    std::vector<Liveness> &files);
} // namespace pepp::link