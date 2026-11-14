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
#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <variant>
#include <vector>
#include "core/formats/elf/packed_access_headers.hpp"
#include "core/formats/elf/packed_ops.hpp"
#include "core/io/mapped_file.hpp"
#include "core/langs/ldscript/ir.hpp"
#include "core/link/diagnostics.hpp"
#include "core/link/link_view.hpp"

namespace pepp::link {

// Assign views over file to each output section's data. Then copy all input sections' data into the corresponding
// output sections.
template <ElfBits B, ElfEndian E>
void write_sections(const LinkView<B, E> &link, bts::PackedGrowableElfFile<B, E> &elf, bts::MappedFile &file) {
  const auto members = link.placements.members();
  for (u32 id = 1; id < link.outputs.size(); ++id) {
    const auto index = link.outputs[id]->shndx;
    const auto &header = elf.section_headers[index];
    // Skip output sections that were not written, which have header 0, and those with no bytes in the file.
    if (index == 0 || header.sh_size == 0 || header.sh_type == bits::to_underlying(bts::SectionTypes::SHT_NOBITS))
      continue;
    const auto slice = file.slice(header.sh_offset, header.sh_size);
    auto &data = elf.section_data[index] = std::make_shared<bts::MemoryMapped>(slice);
    for (const auto position : members.members(OutputSectionID{id})) {
      const auto member = link.placements.sections().handle(position);
      const auto contents = link.group.data(member);
      const auto from = contents->get(0, contents->size());
      const auto to = data->get(link.placements.assignments()[position].offset, from.size());
      if (to.size() != from.size()) throw std::runtime_error("could not map an output section");
      std::ranges::copy(from, to.begin());
    }
    slice->flush();
  }
}

// Write an ET_EXEC with each output section that has contents, in script order. A PT_LOAD is created for each
// run of allocated sections. Path is the destination executable file. If path is nullopt, the mapping is anonymous.
// Returns nullptr if the output cannot be mapped.
template <ElfBits B, ElfEndian E>
std::shared_ptr<bts::MappedFile> create_executable_output(Diagnostics &diagnostics, const LinkView<B, E> &link,
                                                          tc::ldscript::Sections &sections,
                                                          const std::optional<std::filesystem::path> &path) {
  using namespace bts;
  using enum SectionTypes;
  // The first file decided the group's machine, so it decides the output's too.
  const auto &first = link.group.file(ElfFileID(1)).header;
  auto elf = std::make_unique<PackedGrowableElfFile<B, E>>(
      ElfFileType::ET_EXEC, static_cast<ElfMachineType>(u16(first.e_machine)), first.ei_osabi());
  elf->header.e_flags = first.e_flags;
  ensure_section_header_table(*elf);

  // Create section headers for every output section.
  const auto from = static_cast<u16>(elf->section_headers.size());
  for (auto &command : sections.commands) {
    auto *output = std::get_if<tc::ldscript::OutputSectionDescription>(&command);
    if (!output || output->empty) continue;
    // An output section without members still occupies memory when it moved the location counter.
    const bool no_members = output->type == SHT_NULL;
    const auto type = no_members ? SHT_PROGBITS : output->type;
    const u64 flags = no_members ? bits::to_underlying(SectionFlags::SHF_ALLOC) : output->flags;
    const auto index = add_named_section(*elf, output->name, type);
    output->shndx = index;
    auto &header = elf->section_headers[index];
    // An executable keeps no section groups.
    header.sh_flags = static_cast<word<B>>(flags & ~u64(bits::to_underlying(SectionFlags::SHF_GROUP)));
    header.sh_addr = static_cast<word<B>>(output->vma);
    header.sh_addralign = static_cast<word<B>>(output->alignment);

    // write_sections maps the section's data later, so set its size here.
    header.sh_size = static_cast<word<B>>(output->size);
    elf->section_data[index] = nullptr;
  }

  // Compute the minimum set of segments to cover allocated sections.
  std::vector<SegmentLayoutConstraint> segments;
  const auto to = static_cast<u16>(elf->section_headers.size());
  for (const auto &[flags, constraint] : group_load_segments(*elf, from, to)) {
    elf->add_segment(SegmentType::PT_LOAD, flags);
    segments.push_back(constraint);
  }

  // linker-synthesized sections, such as string tables and symbol tables have non-null data need updated sh_size.
  update_section_sizes(*elf);
  const u64 size = assign_file_offsets(*elf, &segments);

  try {
    auto ret = path ? MappedFile::open_truncated(path->string()) : MappedFile::open_anonymous(size);
    // Write the headers. Must release this mapping before the output sections are mapped, since live mappings which
    // overlap lose writes when MappedFile falls back to buffers.
    {
      const auto image = ret->slice(0, size);
      const auto bytes = image->get();
      if (bytes.size() < size) throw std::runtime_error("could not map the output");
      write(bytes, layout_items(*elf));
      image->flush();
    }
    // A new file and an anonymous mapping both read as 0, which fills the gaps between members.
    write_sections(link, *elf, *ret);
    return ret;
  } catch (const std::runtime_error &) {
    const ScriptMessage at{path ? path->string() : std::string()};
    diagnostics.add({Severity::Fatal, LinkCode::CannotWriteOutput, at});
    return nullptr;
  }
}
} // namespace pepp::link
