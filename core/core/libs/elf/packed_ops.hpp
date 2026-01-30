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

#include "core/libs/elf/packed_access_strings.hpp"
#include "core/libs/elf/packed_elf.hpp"
#include "core/libs/elf/packed_storage.hpp"
#include "core/libs/elf/packed_types.hpp"

namespace pepp::bts {

template <ElfBits B, ElfEndian E> void ensure_section_header_table(PackedGrowableElfFile<B, E> &elf);
// Assumes .shstrtab already exists
template <ElfBits B, ElfEndian E>
u16 add_named_section(PackedGrowableElfFile<B, E> &, std::string_view name, SectionTypes type, u16 link = 0);
template <ElfBits B, ElfEndian E>
u16 add_named_symtab(PackedGrowableElfFile<B, E> &, std::string_view name, u16 symtab_idx);
template <ElfBits B, ElfEndian E>
u16 add_named_dynsymtab(PackedGrowableElfFile<B, E> &, std::string_view name, u16 symtab_idx);
template <ElfBits B, ElfEndian E>
u16 add_named_dynamic(PackedGrowableElfFile<B, E> &, std::string_view name, u16 strtab);
template <ElfBits B, ElfEndian E>
u16 add_named_rel(PackedGrowableElfFile<B, E> &, std::string_view name, u16 symtab_idx, u16 section_idx);
template <ElfBits B, ElfEndian E>
u16 add_named_rela(PackedGrowableElfFile<B, E> &, std::string_view name, u16 symtab_idx, u16 section_idx);

template <ElfBits B, ElfEndian E> u32 symbol_count(const PackedElfShdr<B, E> &shdr) {
  if (shdr.sh_entsize == 0 || shdr.sh_size == 0) return 0;
  const u32 size = shdr.sh_size, entsize = shdr.sh_entsize;
  return size / entsize;
}

// Represents a chunk of data to be placed at a specific offset in the final ELF file; it does not own data.
struct LayoutItem {
  u64 offset;
  std::span<const u8> data;
};

// Lay out the ELF header and program header table at a given offset, returning the next available offset.
// Inserts corresponding LayoutItems into the layout vector.
template <ElfBits B, ElfEndian E> u64 place_header_tables_at(PackedElf<B, E> &, std::vector<LayoutItem> &, u64 off);

// Helper to update memory addresses for contiguous sections in a segment automatically.
struct SegmentLayoutConstraint {
  u16 alignment = 0;            // Memory alignment for the segment, must be power-of-two.
  u16 from_sec = 0, to_sec = 0; // File offsets / size will be calculated for these contiguous sections.
  // If true, assign affected section's addresses from base_address upwards.
  // Each section's sh_addr may only be assigned once.
  bool update_sec_addrs = false;
  u64 base_address = 0;
};
// Returns an iovec-like vector of LayoutItems representing the final ELF file layout and associated data.
// The [i]th constraint is for the [ith] segment. If nullptr, no constraints and sh_addr will not be assigned.
// There should never be overlapping [offset, offset+data.size()] entries in a layout, otherwise you have concocted a
// cursed ELF file with illegal offsets and sizes, but this function does not check for those cases.
template <ElfBits B, ElfEndian E>
std::vector<LayoutItem> calculate_layout(PackedElf<B, E> &, const std::vector<SegmentLayoutConstraint> * = nullptr);
// Compute the maximum offset+data.size() in a layout.
u64 size_for_layout(const std::vector<pepp::bts::LayoutItem> &layout) noexcept;
void write(std::span<u8> out, const std::vector<LayoutItem> &layout);

template <ElfBits B, ElfEndian E> void ensure_section_header_table(PackedGrowableElfFile<B, E> &elf) {
  if (!elf.section_headers.empty()) return;
  elf.add_section(create_null_header<B, E>());
  auto idx = elf.add_section(create_shstrtab_header<B, E>(1));

  elf.header.e_shstrndx = 1;
  PackedStringWriter<B, E> writer(elf, elf.header.e_shstrndx);
  static const char hdr[] = ".shstrtab";
  writer.add_string(bits::span<const char>(hdr));
}

template <ElfBits B, ElfEndian E>
u16 add_named_section(PackedGrowableElfFile<B, E> &elf, std::string_view name, SectionTypes type, u16 link) {
  PackedElfShdr<B, E> shdr;
  PackedStringWriter<B, E> writer(elf, elf.header.e_shstrndx);
  shdr.sh_type = to_underlying(type);
  shdr.sh_name = writer.add_string(name);
  shdr.sh_link = link;
  return elf.add_section(std::move(shdr));
}

template <ElfBits B, ElfEndian E>
u16 add_named_symtab(PackedGrowableElfFile<B, E> &elf, std::string_view name, u16 strtab_idx) {
  PackedElfShdr<B, E> shdr;
  PackedStringWriter<B, E> writer(elf, elf.header.e_shstrndx);
  shdr.sh_type = to_underlying(SectionTypes::SHT_SYMTAB);
  shdr.sh_name = writer.add_string(name);
  shdr.sh_link = strtab_idx;
  return elf.add_section(std::move(shdr));
}
template <ElfBits B, ElfEndian E>
u16 add_named_dynsymtab(PackedGrowableElfFile<B, E> &elf, std::string_view name, u16 strtab_idx) {
  PackedElfShdr<B, E> shdr;
  PackedStringWriter<B, E> writer(elf, elf.header.e_shstrndx);
  shdr.sh_type = to_underlying(SectionTypes::SHT_DYNSYM);
  shdr.sh_name = writer.add_string(name);
  shdr.sh_link = strtab_idx;
  return elf.add_section(std::move(shdr));
}
template <ElfBits B, ElfEndian E>
u16 add_named_dynamic(PackedGrowableElfFile<B, E> &elf, std::string_view name, u16 strtab_idx) {
  PackedElfShdr<B, E> shdr;
  PackedStringWriter<B, E> writer(elf, elf.header.e_shstrndx);
  shdr.sh_type = to_underlying(SectionTypes::SHT_DYNAMIC);
  shdr.sh_name = writer.add_string(name);
  shdr.sh_link = strtab_idx;
  return elf.add_section(std::move(shdr));
}
template <ElfBits B, ElfEndian E> u16 add_gnu_version(PackedGrowableElfFile<B, E> &elf, u16 symtab_idx) {
  PackedElfShdr<B, E> shdr;
  PackedStringWriter<B, E> writer(elf, elf.header.e_shstrndx);
  shdr.sh_type = to_underlying(SectionTypes::SHT_GNU_versym);
  shdr.sh_name = writer.add_string(".gnu.version");
  shdr.sh_link = symtab_idx;
  shdr.sh_flags |= to_underlying(SectionFlags::SHF_ALLOC);
  shdr.sh_addralign = 2;
  return elf.add_section(std::move(shdr));
}
template <ElfBits B, ElfEndian E> u16 add_gnu_version_r(PackedGrowableElfFile<B, E> &elf, u16 dynstr) {
  PackedElfShdr<B, E> shdr;
  PackedStringWriter<B, E> writer(elf, elf.header.e_shstrndx);
  shdr.sh_type = to_underlying(SectionTypes::SHT_GNU_verneed);
  shdr.sh_name = writer.add_string(".gnu.version_r");
  shdr.sh_link = dynstr;
  shdr.sh_flags |= to_underlying(SectionFlags::SHF_ALLOC);
  shdr.sh_addralign = sizeof(Word<B, E>);
  return elf.add_section(std::move(shdr));
}
template <ElfBits B, ElfEndian E> u16 add_gnu_version_d(PackedGrowableElfFile<B, E> &elf, u16 dynstr) {
  PackedElfShdr<B, E> shdr;
  PackedStringWriter<B, E> writer(elf, elf.header.e_shstrndx);
  shdr.sh_type = to_underlying(SectionTypes::SHT_GNU_verdef);
  shdr.sh_name = writer.add_string(".gnu.version_d");
  shdr.sh_link = dynstr;
  shdr.sh_flags |= to_underlying(SectionFlags::SHF_ALLOC);
  shdr.sh_addralign = sizeof(Word<B, E>);
  return elf.add_section(std::move(shdr));
}

template <ElfBits B, ElfEndian E>
u16 add_named_rel(PackedGrowableElfFile<B, E> &elf, std::string_view name, u16 symtab_idx, u16 section_idx) {
  PackedElfShdr<B, E> shdr;
  PackedStringWriter<B, E> writer(elf, elf.header.e_shstrndx);
  shdr.sh_type = to_underlying(SectionTypes::SHT_REL);
  shdr.sh_name = writer.add_string(name);
  shdr.sh_link = symtab_idx;
  shdr.sh_info = section_idx;
  return elf.add_section(std::move(shdr));
}

template <ElfBits B, ElfEndian E>
u16 add_named_rela(PackedGrowableElfFile<B, E> &elf, std::string_view name, u16 symtab_idx, u16 section_idx) {
  PackedElfShdr<B, E> shdr;
  PackedStringWriter<B, E> writer(elf, elf.header.e_shstrndx);
  shdr.sh_type = to_underlying(SectionTypes::SHT_RELA);
  shdr.sh_name = writer.add_string(name);
  shdr.sh_link = symtab_idx;
  shdr.sh_info = section_idx;
  return elf.add_section(std::move(shdr));
}

// Place the section header followed by the program header table at the given offset
template <ElfBits B, ElfEndian E>
u64 place_header_tables_at(PackedElf<B, E> &elf, std::vector<LayoutItem> &layout, u64 off) {
  using PackedElf = PackedElf<B, E>;
  // Then place section header table
  if (!elf.section_headers.empty()) {
    elf.header.e_shoff = off;
    layout.emplace_back(LayoutItem{off, std::span<u8>(reinterpret_cast<u8 *>(elf.section_headers.data()),
                                                      sizeof(typename PackedElf::Shdr) * elf.section_headers.size())});
    off += elf.header.e_shentsize * elf.header.e_shnum;
  }
  // Followed by program header table
  if (!elf.program_headers.empty()) {
    elf.header.e_phoff = off;
    layout.emplace_back(LayoutItem{off, std::span<u8>(reinterpret_cast<u8 *>(elf.program_headers.data()),
                                                      sizeof(typename PackedElf::Phdr) * elf.program_headers.size())});
    off += elf.header.e_phentsize * elf.header.e_phnum;
  }
  return off;
}

template <ElfBits B, ElfEndian E>
std::vector<LayoutItem> calculate_layout(PackedElf<B, E> &elf,
                                         const std::vector<SegmentLayoutConstraint> *constraints) {
  // Align an address to an arbitrary alignment. If alignment is 0 or 1, address is returned unchanged.
  constexpr static auto align_to = [](u64 addr, u32 align) {
    if (align < 2) return addr;
    return ((addr + (align - 1)) / align) * align;
  };

  using PackedElf = PackedElf<B, E>;
  std::vector<LayoutItem> ret;
  // std::vector<bool> poor-mans dynamic bitset
  // true if a section has already been assigned an address by a segment constraint.
  // This prevents a single section from having it's sh_addr set to two conflicting values.
  std::vector<bool> touched_sections(elf.section_headers.size(), 0);
  // Segments require that sections start at sh_offset % page_size == p_vaddr % page_size
  // Aligning the file offset to max(segment.p_align, section.sh_addralign) should satisfy this requirement.
  std::vector<u16> over_align(elf.section_headers.size(), 0);
  // 3 is a magic constant including: ehdr, shdr, and phdr
  ret.reserve(elf.section_headers.size() + 2);
  ret.emplace_back(LayoutItem{0, std::span<u8>(reinterpret_cast<u8 *>(&elf.header), sizeof(typename PackedElf::Ehdr))});
  u64 rolling_offset = sizeof(typename PackedElf::Ehdr);
  rolling_offset = place_header_tables_at(elf, ret, rolling_offset);

  // Sections at the start
  if (constraints) {
    for (const auto &constraint : *constraints) {
      if (constraint.from_sec == 0) continue; // Looks like an invalid constraint
      else if (auto &shdr = elf.section_headers[constraint.from_sec]; constraint.alignment > shdr.sh_addralign)
        over_align[constraint.from_sec] = constraint.alignment;
    }
  }

  // Finalize header fields for program header table, excluding memory addresses.
  // Memory addresses will be assigned while evaluating section constraints.
  for (size_t i = 0; i < elf.section_headers.size(); ++i) {
    auto &shdr = elf.section_headers[i];
    // Align rolling offset to the largest of: section alignment or an over-alignment required by a segment constraint
    if (over_align[i] > shdr.sh_addralign) rolling_offset = align_to(rolling_offset, over_align[i]);
    else if (shdr.sh_addralign > 1) rolling_offset = align_to(rolling_offset, shdr.sh_addralign);

    const auto size = elf.section_data[i]->size();
    shdr.sh_offset = rolling_offset, shdr.sh_size = size;
    rolling_offset = elf.section_data[i]->calculate_layout(ret, shdr.sh_offset);
  }

  // Apply segment layout constraints if provided
  if (constraints != nullptr) {
    assert(constraints->size() <= elf.program_headers.size());
    for (u16 it = 0; it < constraints->size(); ++it) {
      auto constraint = (*constraints)[it];
      if (constraint.from_sec == 0) continue; // Looks like an invalid constraint
      auto &phdr = elf.program_headers[it];

      phdr.p_offset = elf.section_headers[constraint.from_sec].sh_offset;
      phdr.p_filesz = elf.section_headers[constraint.to_sec].sh_offset +
                      elf.section_headers[constraint.to_sec].sh_size -
                      elf.section_headers[constraint.from_sec].sh_offset;
      phdr.p_align = constraint.alignment;
      u64 base_address = align_to(constraint.base_address, constraint.alignment);
      phdr.p_vaddr = phdr.p_paddr = base_address;

      for (u16 jt = constraint.from_sec; jt <= constraint.to_sec && jt < elf.section_headers.size(); ++jt) {
        if (touched_sections[jt]) continue; // Already assigned an address
        auto &shdr = elf.section_headers[jt];
        // Set section address
        shdr.sh_addr = base_address;
        base_address += align_to((u64)shdr.sh_size, shdr.sh_addralign);
        touched_sections[jt] = true;
      }
      // Pad memory size to alignment
      u64 memsz = elf.section_headers[constraint.to_sec].sh_addr +
                  align_to(elf.section_headers[constraint.to_sec].sh_size, constraint.alignment) - phdr.p_vaddr;
      // Not all segment types have memory size. For now, only consider PT_LOAD
      if (phdr.p_type == to_underlying(SegmentType::PT_LOAD)) phdr.p_memsz = align_to(memsz, phdr.p_align);
    }
  }

  return ret;
}

} // namespace pepp::core
