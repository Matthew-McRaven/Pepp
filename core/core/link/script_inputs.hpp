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
#include "core/langs/ldscript/ir.hpp"
#include "core/link/diagnostics.hpp"
#include "core/link/link_config.hpp"

namespace pepp::link {
struct ParsedScripts {
  // The effective inputs to the linker, with scripts replaced by their input files.
  std::vector<LinkInput> inputs;
  // The effective linker script, which is every script's commands concatenated in input order.
  tc::ldscript::Script script;
};

// Parse every linker script in inputs. Returns the effective linker script and the updated inputs, with each script
// replaced by the files of its INPUT/GROUP commands. Treats a path that is neither an ELF file nor an archive as a
// script. Prepends a default script if no input is a LinkInput::Script (from -T). Stops at the first script that cannot
// be read or parsed and reports it as Fatal error.
ParsedScripts parse_scripts(Diagnostics &diagnostics, std::vector<LinkInput> inputs);
} // namespace pepp::link