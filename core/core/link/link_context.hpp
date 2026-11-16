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
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "core/formats/elf/packed_input_group.hpp"
#include "core/io/mapped_file.hpp"
#include "core/langs/ldscript/ir.hpp"
#include "core/link/diagnostics.hpp"
#include "core/link/section_placements.hpp"
#include "core/link/symbol_resolution.hpp"

namespace pepp::link {
// The inputs of a link and the state shared between its passes.
struct LinkContext {
  // Null if the link stopped before reading its inputs.
  bts::AnyElfGroup group;
  Diagnostics diagnostics;
  // Indexed by ConfigInputID. Each input's path, or <memory>.
  std::vector<std::string> input_names;
  // Concatenated contents of parsed input linker scripts in input order.
  tc::ldscript::Script script;
  InputSymbols inputs;
  // Indexed by file ID. Index 0 is unused.
  std::vector<Liveness> files;
  ResolvedSymbols symbols;
  // The output section of each placeable input section. Empty when the link stopped before placement.
  std::optional<SectionPlacements> placements;
  // The linked executable. It is the file at LinkConfig::output, or an anonymous mapping otherwise. Null when
  // the link stopped before it was created, or had errors.
  std::shared_ptr<bts::MappedFile> output;

  // True if the link produced no Error or Fatal diagnostics.
  bool ok() const { return diagnostics.count(Severity::Error) == 0; }
};

} // namespace pepp::link