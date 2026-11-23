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
#include <cstddef>
#include <expected>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "core/driver/asmb_driver.hpp"
#include "core/formats/elf/packed_elf.hpp"
#include "core/link/link_config.hpp"
#include "core/link/link_context.hpp"

namespace pepp::tc {

// A source file assembled into an ET_REL object.
struct AssemblySource {
  // Appears in link diagnostics and the link map.
  std::string name;
  DriverConfig config;
  FormattingConfig formatting;
  std::string source;
};

// Link together multiple assembler files into a single ET_EXEC.
struct ExecutableSpec {
  std::vector<AssemblySource> sources;
  link::LinkConfig link;
};

struct ExecutableResult {
  // Indexed like ExecutableSpec::sources.
  std::vector<DriverResult> assemblies;
  // Absent if any source failed to assemble.
  std::optional<link::LinkContext> link;

  bool ok() const;
  // The linked executable. Must be ok().
  std::shared_ptr<const bts::AElfSource> executable() const;
  bts::AnyInputElf open() const;
  // Format sources[index]'s listing with linked addresses and bytes.
  std::expected<std::vector<std::string>, std::string> listing(std::size_t index);
};

// Assemble every source as ET_REL. If every source assembles, link the objects.
ExecutableResult assemble_and_link(ExecutableSpec spec);

} // namespace pepp::tc
