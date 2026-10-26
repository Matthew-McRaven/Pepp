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
#include <algorithm>
#include <numeric>
#include <span>
#include <stdexcept>
#include <vector>
#include "core/ds/glob/glob.hpp"
#include "core/ds/opaque_handle.hpp"
#include "core/formats/elf/packed_ops.hpp"
#include "core/formats/elf/packed_section_selection.hpp"

namespace pepp::link {
using bts::ElfBits;
using bts::ElfEndian;
using bts::InputElfSectionHandle;

// Identifies one output section. Numbered from 1 in output section allocation order.
using OutputSectionID = pepp::OpaqueHandle<struct OutputSectionTag, u32>;
// Identifies one input section match, such as `*(.text)` in a linker script and used to order input sections within an
// output section.
using InputSectionDescriptionID = pepp::OpaqueHandle<struct InputSectionDescriptionTag, u32>;

// Record to which output section an input section is mapped plus a key by which to sort the output(description). The
// input section is implicit.
struct SectionClaim {
  OutputSectionID output;
  InputSectionDescriptionID description;

  auto operator<=>(const SectionClaim &) const = default;
  bool operator==(const SectionClaim &) const = default;
  // An invalid output means unclaimed.
  constexpr explicit operator bool() const { return bool(output); }
};

template <ElfBits B, ElfEndian E> class SectionMembers;

// Given a selection over input sections (usually constructed via a glob), record which input section description first
// claimed each input section. The first selection to cover an input section prevents later selections from claiming it,
// which matches the typical linker rules. If the InputGroup changes, this class needs to be re-generated.
template <ElfBits B, ElfEndian E> class SectionPlacements {
public:
  using Group = bts::PackedInputElfGroup<B, E>;
  using Selection = bts::PackedSectionSelection<B, E>;

  explicit SectionPlacements(const Group &group) : _sections(group), _claims(_sections.size()) {}

  // All input sections covered by this table.
  const Selection &sections() const noexcept { return _sections; }
  // The set of claimed input sections, which share an index with _sections.
  std::span<const SectionClaim> claims() const noexcept { return _claims; }
  // Throws std::out_of_range if this table does not cover the section.
  SectionClaim claim_of(InputElfSectionHandle handle) const { return _claims[position_of(handle)]; }
  bool claimed(InputElfSectionHandle handle) const { return bool(claim_of(handle)); }

  // Claim every section of selection that is still unclaimed, skipping the rest, and return how many were claimed.
  // Throws std::invalid_argument if output is invalid.
  u32 claim(const Selection &selection, OutputSectionID output, InputSectionDescriptionID description);
  // The claimed sections, grouped by output section. Build it once every description is matched.
  SectionMembers<B, E> members() const;

private:
  u32 position_of(InputElfSectionHandle handle) const;
  Selection _sections;
  std::vector<SectionClaim> _claims;
};

// The sorted input sections of each output section. Sorted first by description, then by file order in group, then by
// section index within their file. This should match GNU ld's sorting.
template <ElfBits B, ElfEndian E> class SectionMembers {
public:
  // Empty for an unknown ID.
  std::span<const InputElfSectionHandle> members(OutputSectionID output) const;

private:
  friend class SectionPlacements<B, E>;
  explicit SectionMembers(const SectionPlacements<B, E> &placements);
  std::vector<InputElfSectionHandle> _members;
  // _members[_offsets[id], _offsets[id + 1]) are output section id's members. Index 0 is unused.
  std::vector<u32> _offsets;
};

template <ElfBits B, ElfEndian E>
u32 SectionPlacements<B, E>::claim(const Selection &selection, OutputSectionID output,
                                   InputSectionDescriptionID description) {
  if (!output) throw std::invalid_argument("SectionPlacements: output must be valid");
  u32 claimed = 0;
  for (const auto handle : selection.handles()) {
    auto &claim = _claims[position_of(handle)];
    if (claim) continue;
    claim = {output, description};
    ++claimed;
  }
  return claimed;
}

template <ElfBits B, ElfEndian E> SectionMembers<B, E> SectionPlacements<B, E>::members() const {
  return SectionMembers<B, E>(*this);
}

template <ElfBits B, ElfEndian E> u32 SectionPlacements<B, E>::position_of(InputElfSectionHandle handle) const {
  const auto position = _sections.position(handle);
  if (!position) throw std::out_of_range("SectionPlacements: section not covered");
  return *position;
}

template <ElfBits B, ElfEndian E> SectionMembers<B, E>::SectionMembers(const SectionPlacements<B, E> &placements) {
  const auto claims = placements.claims();
  const auto handles = placements.sections().handles();
  // Collect the indices of all claimed sections and track total number of output sections.
  std::vector<u32> positions;
  u32 outputs = 0;
  for (u32 position = 0; position < claims.size(); ++position) {
    if (!claims[position]) continue;
    positions.push_back(position);
    outputs = std::max(outputs, claims[position].output.value);
  }
  // Sort by output section then description. Ties within a description preserve the group's order,
  // which gives us sorting by file order and section index order within a file for free.
  std::ranges::stable_sort(positions, {}, [&](u32 position) { return claims[position]; });

  // For each id members span [_offsets[id], _offsets[id + 1]).
  _members.reserve(positions.size());
  // Slot 0 is unused, and we need id+1 entries to prevent an buffer overrun when accessing the maximum id, so we need
  // to zero a total of outputs+2 elements.
  _offsets.assign(outputs + 2, 0);
  // Count the total number of members in each output section.
  for (const auto position : positions) {
    _members.push_back(handles[position]);
    ++_offsets[claims[position].output.value];
  }
  // Converts from a count at each output section to an offset in our _members array.
  // _offsets[i] becomes the sum of the counts before it, _offsets[0..i), which is where output i starts.
  std::exclusive_scan(_offsets.begin(), _offsets.end(), _offsets.begin(), 0u);
}

template <ElfBits B, ElfEndian E>
std::span<const InputElfSectionHandle> SectionMembers<B, E>::members(OutputSectionID output) const {
  if (!output || output.value >= _offsets.size() - 1) return {};
  const u32 first = _offsets[output.value];
  return std::span(_members).subspan(first, _offsets[output.value + 1] - first);
}

// The sections whose names match glob and which placements has not claimed yet. The null section is never selected.
// The glob is mutable because it compiles itself on first use.
template <ElfBits B, ElfEndian E>
bts::PackedSectionSelection<B, E> select_unclaimed(const bts::PackedInputElfGroup<B, E> &group, Glob &glob,
                                                   const SectionPlacements<B, E> &placements) {
  auto f = [&](const bts::InputElfSectionRef<B, E> &ref) {
    return ref.handle.index != 0 && !placements.claimed(ref.handle) &&
           glob.find(bts::section_name(ref.file, ref.handle.index)) >= 0;
  };
  return bts::PackedSectionSelection<B, E>(group, f);
}
} // namespace pepp::link
