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

#include "core/link/map_file.hpp"
#include <algorithm>
#include <fstream>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>
#include <fmt/format.h>
#include "core/link/link_context.hpp"

namespace {
namespace ld = pepp::tc::ldscript;
using pepp::link::LinkContext;
// Align columns to 16 chars.
const std::string indent(16, ' ');
const ld::Sections *sections_of(const ld::Script &script) {
  for (const auto &command : script.commands)
    if (const auto *ret = std::get_if<ld::Sections>(&command)) return ret;
  return nullptr;
}

std::string format_row(std::string name, u64 address, u64 size, int digits) {
  // ld gives long names a line of their own to keep columns aligned.
  if (name.size() < indent.size() - 1) name.append(indent.size() - name.size(), ' ');
  else name += '\n' + indent;
  return fmt::format("{}0x{:0{}x} {:>10}", name, address, digits, fmt::format("{:#x}", size));
}

// Format the "Linker script and memory map", which lists each output section. globals is indexed like
// SectionPlacements::sections().
std::string memory_map(const LinkContext &context, const ld::Sections &sections,
                       const std::vector<std::vector<std::pair<u64, std::string_view>>> &globals, int digits) {
  const auto &summary = *context.summary;
  const auto &selection = context.placements->sections();
  const auto assignments = context.placements->assignments();
  const auto members = context.placements->members();
  std::string ret = "\nLinker script and memory map\n\n";
  for (const auto &command : sections.commands) {
    const auto *output = std::get_if<ld::OutputSectionDescription>(&command);
    if (!output || output->discard) continue;
    const auto positions = members.members(output->id);
    // GNU ld reports no address for an output section which was left out of the file and has no members.
    const bool unplaced = output->empty && positions.empty();
    ret += fmt::format("\n{}\n", unplaced ? output->name : format_row(output->name, output->vma, output->size, digits));
    auto next = positions.begin();
    for (const auto &inner : output->commands) {
      const auto *description = std::get_if<ld::InputSectionDescription>(&inner);
      if (!description) continue;
      // GNU ld lists orphans without a description
      if (!description->orphan) ret += fmt::format(" {}\n", ld::to_string(*description));
      // Members are sorted by description, so this description's members come next.
      for (; next != positions.end() && assignments[*next].description == description->id; ++next) {
        const auto file = selection.handle(*next).file;
        const auto name = " " + summary.section_names[*next];
        const auto address = summary.section_addresses[*next].value_or(0);
        ret += fmt::format("{} {}\n", format_row(name, address, summary.section_sizes[*next], digits),
                           summary.file_names[file.value]);
        for (const auto &[value, symbol] : globals[*next])
          ret += fmt::format("{}0x{:0{}x}{}{}\n", indent, value, digits, indent, symbol);
      }
    }
  }
  return ret;
}
} // namespace

std::optional<std::string> pepp::link::link_map(const LinkContext &context) {
  const auto *sections = sections_of(context.script);
  if (!sections || !context.placements || !context.summary) return std::nullopt;
  const int digits = context.summary->bits == ElfBits::b64 ? 16 : 8;

  // The address and name of each global symbol, grouped by the position of the defining input section.
  std::vector<std::vector<std::pair<u64, std::string_view>>> globals(context.placements->sections().size());
  for (u32 it = 1; it < context.symbols.size(); ++it) {
    const SymbolID id(it);
    const auto owner = context.symbols.symbol(id).owner;
    // Symbols participating in forwarding (e.g., foo@V forwards to foo) are only listed under the forwarded-to symbol.
    if (!owner || context.symbols.of(owner) != id) continue;
    else if (const auto position = context.summary->defined_in[it])
      globals[*position].emplace_back(context.definitions[it].address, context.symbols.name(id));
  }
  for (auto &defined : globals) std::ranges::stable_sort(defined, {}, &std::pair<u64, std::string_view>::first);

  return memory_map(context, *sections, globals, digits);
}

void pepp::link::write_link_map(LinkContext &context, const std::filesystem::path &path) {
  const auto map = link_map(context);
  std::ofstream file(path, std::ios::binary | std::ios::trunc);
  if (map) file.write(map->data(), static_cast<std::streamsize>(map->size()));
  file.close();
  if (!file) context.diagnostics.add({Severity::Error, LinkCode::CannotWriteOutput, ScriptMessage{path.string()}});
}

std::optional<u64> pepp::link::section_address(const LinkContext &context, bts::InputElfSectionHandle section) {
  if (!context.placements || !context.summary) return std::nullopt;
  else if (const auto position = context.placements->sections().position(section))
    return context.summary->section_addresses[*position];
  return std::nullopt;
}
