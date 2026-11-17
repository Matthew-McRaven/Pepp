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
#include "core/link/map_file.hpp"
#include "core/link/section_placements.hpp"
#include "core/link/symbol_resolution.hpp"

namespace pepp::link {
// The inputs of a link and the state shared between its passes.
// All pointers and optional values may be null/empty if the link failed with a fatal error.
struct LinkContext {
  // Null if the link stopped before reading its inputs or after close_output() for non-anonymous outputs.
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
  // Indexed by SymbolID. Empty until resolved_symbol_definitions runs after layout.
  std::vector<OutputSymbolDefinition> definitions;
  // The output section of each placeable input section.
  std::optional<SectionPlacements> placements;
  // The linked executable as a mapping of the file at LinkConfig::output, or an anonymous mapping
  // otherwise. May be read-only or read-write, but you should only
  // use it as-if read-only.
  std::shared_ptr<bts::MappedFile> output;
  // Portions of the inputs used by link_map() than need to outlive the input files being closed.
  // The part of the inputs read by link_map(), which outlives close_output().
  std::unique_ptr<InputSummary> summary;

  // True if the link produced no Error or Fatal diagnostics.
  bool ok() const { return diagnostics.count(Severity::Error) == 0; }
};

} // namespace pepp::link