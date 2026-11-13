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
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "core/ds/glob/glob.hpp"
#include "core/ds/opaque_handle.hpp"
#include "core/formats/elf/packed_ops.hpp"
#include "core/formats/elf/packed_section_selection.hpp"
#include "core/langs/ldscript/ir.hpp"
#include "core/link/symbol_resolution.hpp"

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
struct SectionAssignment {
  OutputSectionID output;
  InputSectionDescriptionID description;

  auto operator<=>(const SectionAssignment &) const = default;
  bool operator==(const SectionAssignment &) const = default;
  // An invalid output means unassigned.
  constexpr explicit operator bool() const { return bool(output); }
};

class SectionMembers;

// For a selection of input sections, record the output section to which it is assigned. Like the underlying
// PackedSectionSelection, this class needs to be regenerated if the InputElfGroup changes.
class SectionPlacements {
public:
  using Selection = bts::PackedSectionSelection;

  // Covers only the sections of the selection.
  explicit SectionPlacements(Selection sections) : _sections(std::move(sections)), _assignments(_sections.size()) {}

  // All input sections covered by this table.
  const Selection &sections() const noexcept { return _sections; }
  // The assignment of each input section, which shares an index with _sections.
  std::span<const SectionAssignment> assignments() const noexcept { return _assignments; }
  // Throws std::out_of_range if this table does not cover the section.
  SectionAssignment assignment_of(InputElfSectionHandle handle) const { return _assignments[position_of(handle)]; }

  // Assign a section that is not yet assigned. Throws std::invalid_argument if output is invalid.
  void assign(InputElfSectionHandle handle, OutputSectionID output, InputSectionDescriptionID description);
  // The assigned sections, grouped by output section. Build it once every description is matched.
  SectionMembers members() const;

private:
  u32 position_of(InputElfSectionHandle handle) const;
  Selection _sections;
  std::vector<SectionAssignment> _assignments;
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

// Represents an output section, which is indexed by its OutputSectionID.
struct OutputSection {
  std::string name;
  // The address given to this section by a script (e.g., .text 0x1000 : { ... }),  evaluated when addresses are
  // assigned since it may depend on . and on earlier sections and symbols.
  std::optional<tc::expr::Tree> address;
};

// Select the sections of live files, except null sections and those the linker consumes (e.g., symbol tables).
template <ElfBits B, ElfEndian E>
bts::PackedSectionSelection placeable_sections(const bts::PackedInputElfGroup<B, E> &group,
                                               std::span<const Liveness> files) {
  auto f = [&](const bts::InputElfSectionRef<B, E> &ref) {
    using enum bts::SectionTypes;
    switch (static_cast<bts::SectionTypes>(u32(ref.header.sh_type))) {
    case SHT_NULL: [[fallthrough]];
    case SHT_SYMTAB: [[fallthrough]];
    case SHT_STRTAB: [[fallthrough]];
    case SHT_RELA: [[fallthrough]];
    case SHT_REL: [[fallthrough]];
    case SHT_GROUP: [[fallthrough]];
    case SHT_SYMTAB_SHNDX: return false;
    default: return files[ref.handle.file.value] == Liveness::Live;
    }
  };
  return bts::PackedSectionSelection(group, f);
}

// Represents the result of place_sections().
struct PlacedSections {
  // Indexed by OutputSectionID. Index 0 is unused.
  std::vector<OutputSection> outputs;
  SectionPlacements placements;
};

// Represents an input section description whose file and section patterns have been compiled.
class InputSectionDescriptionMatcher {
public:
  explicit InputSectionDescriptionMatcher(const tc::ldscript::InputSectionDescription &description);
  // Returns true if both the section's file and its name match.
  // TODO: if file matching consumes a large fraction of runtime, implement a filename cache at the place_sections
  // level.
  template <ElfBits B, ElfEndian E>
  bool matches(const bts::PackedInputElfGroup<B, E> &group, InputElfSectionHandle section) const {
    return matches_file(group, section.file) &&
           _sections.find(bts::section_name(group.file(section.file), section.index)) >= 0;
  }

private:
  // Returns true if the file pattern matches the file's name or an archive's member.
  template <ElfBits B, ElfEndian E>
  bool matches_file(const bts::PackedInputElfGroup<B, E> &group, bts::ElfFileID file) const {
    using enum tc::ldscript::FilePattern::Match;
    const auto &provenance = group.provenance(file);
    if (const auto *member = std::get_if<bts::ElfFromArchive>(&provenance)) {
      const auto &archive = group.archive(member->archive);
      const auto archive_path = archive.path() ? archive.path()->string() : std::string();
      if (_match == WholeArchive) return _file.find(archive_path) >= 0;
      return _match == Name && _file.find(archive_path + ":" + std::string(archive.name(member->member))) >= 0;
    }
    // A file in memory has an empty name.
    const auto *path = std::get_if<bts::ElfFromPath>(&provenance);
    return _match != WholeArchive && _file.find(path ? path->path.string() : std::string()) >= 0;
  }

  tc::ldscript::FilePattern::Match _match;
  // Glob compiles itself on first use, but I want this class to act as-if const.
  mutable Glob _file, _sections;
};

// Iterate over the live input sections. If the descriptor matches a given (section+file) and the section is unassigned,
// map the input section to the current output section.
template <ElfBits B, ElfEndian E>
void assign_matching(const bts::PackedInputElfGroup<B, E> &group, SectionPlacements &placements,
                     const tc::ldscript::InputSectionDescription &description, OutputSectionID output,
                     InputSectionDescriptionID id) {
  InputSectionDescriptionMatcher matcher(description);
  for (const auto section : placements.sections().handles())
    if (!placements.assignment_of(section) && matcher.matches(group, section)) placements.assign(section, output, id);
}

// Assign each placeable section to the first input section description in script that matches it. Output sections with
// duplicate names will receive unique section ID.
template <ElfBits B, ElfEndian E>
PlacedSections place_sections(const bts::PackedInputElfGroup<B, E> &group, std::span<const Liveness> files,
                              const tc::ldscript::Script &script) {
  // Index 0 is unused, since OutputSectionID 0 is invalid.
  PlacedSections ret{{OutputSection{}}, SectionPlacements(placeable_sections(group, files))};
  u32 descriptions = 0;
  for (const auto *statement : tc::ldscript::output_sections(script)) {
    ret.outputs.push_back({statement->name, statement->address});
    const OutputSectionID output{static_cast<u32>(ret.outputs.size() - 1)};
    for (const auto &item : statement->items)
      if (const auto *input = std::get_if<tc::ldscript::InputSectionDescription>(&item))
        assign_matching(group, ret.placements, *input, output, InputSectionDescriptionID{++descriptions});
  }
  return ret;
}
} // namespace pepp::link
