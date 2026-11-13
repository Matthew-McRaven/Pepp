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

class SectionMembers;

// Given a selection over input sections (usually constructed via a glob), record which input section description first
// claimed each input section. The first selection to cover an input section prevents later selections from claiming it,
// which matches the typical linker rules. If the InputGroup changes, this class needs to be re-generated.
class SectionPlacements {
public:
  using Selection = bts::PackedSectionSelection;

  template <ElfBits B, ElfEndian E>
  explicit SectionPlacements(const bts::PackedInputElfGroup<B, E> &group)
      : _sections(group), _claims(_sections.size()) {}

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
  SectionMembers members() const;

private:
  u32 position_of(InputElfSectionHandle handle) const;
  Selection _sections;
  std::vector<SectionClaim> _claims;
};

// The sorted input sections of each output section. Sorted first by description, then by file order in group, then by
// section index within their file. This should match GNU ld's sorting.
class SectionMembers {
public:
  // Empty for an unknown ID.
  std::span<const InputElfSectionHandle> members(OutputSectionID output) const;

private:
  friend class SectionPlacements;
  explicit SectionMembers(const SectionPlacements &placements);
  std::vector<InputElfSectionHandle> _members;
  // _members[_offsets[id], _offsets[id + 1]) are output section id's members. Index 0 is unused.
  std::vector<u32> _offsets;
};

// The sections whose names match glob and which placements has not claimed yet. The null section is never selected.
// The glob is mutable because it compiles itself on first use.
template <ElfBits B, ElfEndian E>
bts::PackedSectionSelection select_unclaimed(const bts::PackedInputElfGroup<B, E> &group, Glob &glob,
                                             const SectionPlacements &placements) {
  auto f = [&](const bts::InputElfSectionRef<B, E> &ref) {
    return ref.handle.index != 0 && !placements.claimed(ref.handle) &&
           glob.find(bts::section_name(ref.file, ref.handle.index)) >= 0;
  };
  return bts::PackedSectionSelection(group, f);
}
} // namespace pepp::link
