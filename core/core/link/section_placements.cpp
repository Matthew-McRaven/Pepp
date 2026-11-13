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
