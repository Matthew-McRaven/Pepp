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

#include "core/link/section_placements.hpp"

namespace {
namespace ld = pepp::tc::ldscript;

// Returns the index of the first output section in commands that satisfies pred, or nullopt if there is none.
std::optional<size_t> first_output_section(std::span<const ld::Sections::Command> commands, auto pred) {
  for (size_t index = 0; index < commands.size(); ++index)
    if (const auto *output = std::get_if<ld::OutputSectionDescription>(&commands[index]); output && pred(*output))
      return index;
  return std::nullopt;
}

// Returns the index of the last output section in commands that satisfies pred, or nullopt if there is none.
std::optional<size_t> last_output_section(std::span<const ld::Sections::Command> commands, auto pred) {
  for (size_t index = commands.size(); index-- > 0;)
    if (const auto *output = std::get_if<ld::OutputSectionDescription>(&commands[index]); output && pred(*output))
      return index;
  return std::nullopt;
}

// Returns true if command assigns to a symbol other than the location counter (`.`).
bool is_symbol_assignment(const ld::Sections::Command &command) {
  const auto *assignment = std::get_if<ld::Assignment>(&command);
  return assignment && assignment->symbol != ".";
}

// Returns the attributes of an output section's members, or nullopt if it has no members.
std::optional<pepp::link::OrphanAttributes> attributes_of(const ld::OutputSectionDescription &output) {
  if (output.type == pepp::bts::SectionTypes::SHT_NULL) return std::nullopt;
  return pepp::link::orphan_attributes(output.flags, output.type);
}

// Returns the index of the output section after which an orphan should be placed.
std::optional<size_t> preceding_output_section(std::span<const ld::Sections::Command> commands,
                                               pepp::link::OrphanAttributes attributes) {
  static constexpr std::array<std::string_view, 4> usual_names{".text", ".rodata", ".data", ".bss"};
  const auto same = [&](const auto &output) { return attributes_of(output) == attributes; };
  // After the previous orphan with the same attributes, which preserves input order among orphans.
  if (const auto at = last_output_section(commands, [&](const auto &output) { return output.orphan && same(output); }))
    return at;
  // Then after a standard section which shares the same attributes.
  const auto usual_name = usual_names[std::to_underlying(attributes)];
  if (const auto at = first_output_section(commands, [&](const auto &output) { return output.name == usual_name; }))
    return at;
  // Then after any output section with the same attributes.
  if (const auto at = last_output_section(commands, same)) return at;
  // Lastly after any output section whose attributes place it earlier in the output order.
  return last_output_section(commands, [&](const auto &output) {
    return attributes_of(output).value_or(attributes) < attributes;
  });
}
} // namespace

pepp::link::OrphanAttributes pepp::link::orphan_attributes(u64 flags, bts::SectionTypes type) {
  using enum OrphanAttributes;
  using enum bts::SectionFlags;
  const auto has = [flags](bts::SectionFlags flag) { return (flags & static_cast<u64>(flag)) != 0; };
  if (!has(SHF_ALLOC)) return NonAlloc;
  else if (type == bts::SectionTypes::SHT_NOBITS) return Bss;
  else if (has(SHF_WRITE)) return Data;
  else if (has(SHF_EXECINSTR)) return Text;
  return Rodata;
}

std::optional<size_t> pepp::link::find_output_section(std::span<const tc::ldscript::Sections::Command> commands,
                                                      std::string_view name) {
  return first_output_section(commands, [name](const auto &output) { return output.name == name; });
}

size_t pepp::link::insert_orphan(std::vector<tc::ldscript::Sections::Command> &commands,
                                 tc::ldscript::OutputSectionDescription orphan) {
  const auto attributes = orphan_attributes(orphan.flags, orphan.type);
  // Non-allocated orphans go last.
  auto at = commands.size();
  if (attributes != OrphanAttributes::NonAlloc) {
    if (const auto preceding = preceding_output_section(commands, attributes)) at = *preceding + 1;
    else {
      // Insert after a leading location counter assignment, or else before the first output section.
      at = 0;
      while (at < commands.size() && is_symbol_assignment(commands[at])) ++at;
      if (at < commands.size() && std::holds_alternative<ld::Assignment>(commands[at])) ++at;
    }
    // Skip over symbol assignments, which are often used to mark the end of an output section (e.g., _etext = .;).
    while (at < commands.size() && is_symbol_assignment(commands[at])) ++at;
  }
  commands.emplace(commands.begin() + static_cast<std::ptrdiff_t>(at), std::move(orphan));
  return at;
}

pepp::link::InputSectionDescriptionID pepp::link::orphan_description(tc::ldscript::OutputSectionDescription &output,
                                                                     u32 &descriptions) {
  for (const auto &command : output.commands)
    if (const auto *input = std::get_if<ld::InputSectionDescription>(&command); input && input->orphan)
      return input->id;
  const InputSectionDescriptionID id{++descriptions};
  output.commands.emplace_back(ld::InputSectionDescription{
      .file = {.pattern = "*"}, .sections = {output.name}, .id = id, .orphan = true});
  return id;
}

void pepp::link::SectionPlacements::assign(InputElfSectionHandle handle, OutputSectionID output,
                                           InputSectionDescriptionID description) {
  if (!output) throw std::invalid_argument("SectionPlacements: output must be valid");
  _assignments[position_of(handle)] = {output, description};
}

pepp::link::SectionMembers pepp::link::SectionPlacements::members() const { return SectionMembers(*this); }

u32 pepp::link::SectionPlacements::position_of(InputElfSectionHandle handle) const {
  const auto position = _sections.position(handle);
  if (!position) throw std::out_of_range("SectionPlacements: section not covered");
  return *position;
}

pepp::link::SectionMembers::SectionMembers(const SectionPlacements &placements) {
  const auto assignments = placements.assignments();
  const auto handles = placements.sections().handles();
  // Collect the indices of all assigned sections and track total number of output sections.
  std::vector<u32> positions;
  u32 outputs = 0;
  for (u32 position = 0; position < assignments.size(); ++position) {
    if (!assignments[position]) continue;
    positions.push_back(position);
    outputs = std::max(outputs, assignments[position].output.value);
  }
  // Sort by output section then description. Ties within a description preserve the group's order,
  // which gives us sorting by file order and section index order within a file for free.
  std::ranges::stable_sort(positions, {}, [&](u32 position) { return assignments[position]; });

  // For each id members span [_offsets[id], _offsets[id + 1]).
  _members.reserve(positions.size());
  // Slot 0 is unused, and we need id+1 entries to prevent an buffer overrun when accessing the maximum id, so we need
  // to zero a total of outputs+2 elements.
  _offsets.assign(outputs + 2, 0);
  // Count the total number of members in each output section.
  for (const auto position : positions) {
    _members.push_back(handles[position]);
    ++_offsets[assignments[position].output.value];
  }
  // Converts from a count at each output section to an offset in our _members array.
  // _offsets[i] becomes the sum of the counts before it, _offsets[0..i), which is where output i starts.
  std::exclusive_scan(_offsets.begin(), _offsets.end(), _offsets.begin(), 0u);
}

std::span<const pepp::bts::InputElfSectionHandle> pepp::link::SectionMembers::members(OutputSectionID output) const {
  if (!output || output.value >= _offsets.size() - 1) return {};
  const u32 first = _offsets[output.value];
  return std::span(_members).subspan(first, _offsets[output.value + 1] - first);
}

pepp::link::InputSectionDescriptionMatcher::InputSectionDescriptionMatcher(
    const tc::ldscript::InputSectionDescription &description)
    : _match(description.file.match) {
  _file.add(description.file.pattern, 0);
  for (const auto &section : description.sections) _sections.add(section, 0);
}
