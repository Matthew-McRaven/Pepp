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

#include "core/formats/elf/packed_access_strings.hpp"
#include "core/formats/elf/packed_elf.hpp"
#include "core/formats/elf/packed_storage.hpp"
#include "core/formats/elf/packed_types.hpp"
#include <stdexcept>
#include "core/math/bitmanip/log2.hpp"

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
  // If true, assign the affected sections' addresses from base_address upwards, and each section's
  // sh_addr may only be assigned once. Sections will be padded to their own sh_addralign, and the first section will be
  // aligned to max(sh_addralign, alignment).
  //
  // If false, the sections already carry properly computed addresses, and p_vaddr is taken from the first section.
  // Later sections will be placed according to their offset from the first section's address. Sections placed this way
  // must have asceding, non-overlapping addresses, otherwise the segment is invalid.
  bool update_sec_addrs = true;
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
  shdr.sh_type = bits::to_underlying(type);
  shdr.sh_name = writer.add_string(name);
  shdr.sh_link = link;
  return elf.add_section(std::move(shdr));
}

template <ElfBits B, ElfEndian E>
u16 add_named_symtab(PackedGrowableElfFile<B, E> &elf, std::string_view name, u16 strtab_idx) {
  PackedElfShdr<B, E> shdr;
  PackedStringWriter<B, E> writer(elf, elf.header.e_shstrndx);
  shdr.sh_type = bits::to_underlying(SectionTypes::SHT_SYMTAB);
  shdr.sh_name = writer.add_string(name);
  shdr.sh_link = strtab_idx;
  return elf.add_section(std::move(shdr));
}
template <ElfBits B, ElfEndian E>
u16 add_named_dynsymtab(PackedGrowableElfFile<B, E> &elf, std::string_view name, u16 strtab_idx) {
  PackedElfShdr<B, E> shdr;
  PackedStringWriter<B, E> writer(elf, elf.header.e_shstrndx);
  shdr.sh_type = bits::to_underlying(SectionTypes::SHT_DYNSYM);
  shdr.sh_name = writer.add_string(name);
  shdr.sh_link = strtab_idx;
  return elf.add_section(std::move(shdr));
}
template <ElfBits B, ElfEndian E>
u16 add_named_dynamic(PackedGrowableElfFile<B, E> &elf, std::string_view name, u16 strtab_idx) {
  PackedElfShdr<B, E> shdr;
  PackedStringWriter<B, E> writer(elf, elf.header.e_shstrndx);
  shdr.sh_type = bits::to_underlying(SectionTypes::SHT_DYNAMIC);
  shdr.sh_name = writer.add_string(name);
  shdr.sh_link = strtab_idx;
  return elf.add_section(std::move(shdr));
}
template <ElfBits B, ElfEndian E> u16 add_gnu_version(PackedGrowableElfFile<B, E> &elf, u16 symtab_idx) {
  PackedElfShdr<B, E> shdr;
  PackedStringWriter<B, E> writer(elf, elf.header.e_shstrndx);
  shdr.sh_type = bits::to_underlying(SectionTypes::SHT_GNU_versym);
  shdr.sh_name = writer.add_string(".gnu.version");
  shdr.sh_link = symtab_idx;
  shdr.sh_flags |= bits::to_underlying(SectionFlags::SHF_ALLOC);
  shdr.sh_addralign = 2;
  return elf.add_section(std::move(shdr));
}
template <ElfBits B, ElfEndian E> u16 add_gnu_version_r(PackedGrowableElfFile<B, E> &elf, u16 dynstr) {
  PackedElfShdr<B, E> shdr;
  PackedStringWriter<B, E> writer(elf, elf.header.e_shstrndx);
  shdr.sh_type = bits::to_underlying(SectionTypes::SHT_GNU_verneed);
  shdr.sh_name = writer.add_string(".gnu.version_r");
  shdr.sh_link = dynstr;
  shdr.sh_flags |= bits::to_underlying(SectionFlags::SHF_ALLOC);
  shdr.sh_addralign = sizeof(Word<B, E>);
  return elf.add_section(std::move(shdr));
}
template <ElfBits B, ElfEndian E> u16 add_gnu_version_d(PackedGrowableElfFile<B, E> &elf, u16 dynstr) {
  PackedElfShdr<B, E> shdr;
  PackedStringWriter<B, E> writer(elf, elf.header.e_shstrndx);
  shdr.sh_type = bits::to_underlying(SectionTypes::SHT_GNU_verdef);
  shdr.sh_name = writer.add_string(".gnu.version_d");
  shdr.sh_link = dynstr;
  shdr.sh_flags |= bits::to_underlying(SectionFlags::SHF_ALLOC);
  shdr.sh_addralign = sizeof(Word<B, E>);
  return elf.add_section(std::move(shdr));
}

template <ElfBits B, ElfEndian E>
u16 add_named_rel(PackedGrowableElfFile<B, E> &elf, std::string_view name, u16 symtab_idx, u16 section_idx) {
  PackedElfShdr<B, E> shdr;
  PackedStringWriter<B, E> writer(elf, elf.header.e_shstrndx);
  shdr.sh_type = bits::to_underlying(SectionTypes::SHT_REL);
  shdr.sh_name = writer.add_string(name);
  shdr.sh_link = symtab_idx;
  shdr.sh_info = section_idx;
  return elf.add_section(std::move(shdr));
}

template <ElfBits B, ElfEndian E>
u16 add_named_rela(PackedGrowableElfFile<B, E> &elf, std::string_view name, u16 symtab_idx, u16 section_idx) {
  PackedElfShdr<B, E> shdr;
  PackedStringWriter<B, E> writer(elf, elf.header.e_shstrndx);
  shdr.sh_type = bits::to_underlying(SectionTypes::SHT_RELA);
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
  // Sentinel value if a section does not belong to any segment constraint
  static constexpr u16 NO_OWNER = 0xFFFF;
  using PackedElf = PackedElf<B, E>;

  // The ith constraint is for the ith segment, so there cannot be more constraints than segments.
  if (constraints && constraints->size() > elf.program_headers.size())
    throw std::logic_error("calculate_layout: more segment constraints than segments");

  // 3 is a magic constant including: ehdr, shdr, and phdr
  std::vector<LayoutItem> ret;
  ret.reserve(elf.section_headers.size() + 3);
  ret.emplace_back(LayoutItem{0, std::span<u8>(reinterpret_cast<u8 *>(&elf.header), sizeof(typename PackedElf::Ehdr))});
  // Allocate section, program headers in the ELF file.
  u64 rolling_offset = place_header_tables_at(elf, ret, sizeof(typename PackedElf::Ehdr));

  // Record which constraint (an index) first place a section so that no section can be placed twice.
  std::vector<u16> owner(elf.section_headers.size(), NO_OWNER);
  // Extra alignment a segment imposes on the section it begins at, over that section's own sh_addralign.
  std::vector<u64> over_align(elf.section_headers.size(), 0);

  // Compute over-alignment on sections imposed by segments, and group sections by their owning constraint.
  for (u16 it = 0; constraints && it < constraints->size(); ++it) {
    const auto &constraint = (*constraints)[it];
    // The caller manually edited this segment's header, so leave it alone.
    if (constraint.from_sec == 0) [[unlikely]]
      continue;
    else if (constraint.to_sec >= elf.section_headers.size()) [[unlikely]]
      throw std::logic_error("calculate_layout: segment constraint references nonexistent section");
    else if (constraint.to_sec < constraint.from_sec) [[unlikely]]
      throw std::logic_error("calculate_layout: segment constraint covers no sections");

    // Segments require that sections start at sh_offset % page_size == p_vaddr % page_size
    // Aligning the file offset to max(segment.p_align, section.sh_addralign) should satisfy this requirement.
    if (constraint.update_sec_addrs)
      over_align[constraint.from_sec] = std::max<u64>(constraint.alignment, over_align[constraint.from_sec]);
    // A NOBITS section advances a segment's addresses without advancing its file offsets, so a NOBITS in the middle of
    // a segment will not work as expected.
    bool seen_nobits = false;
    // Count how many sections have already been claimed by an earlier segment.
    size_t already_claimed = 0;
    for (u16 jt = constraint.from_sec; jt <= constraint.to_sec; ++jt) {
      if (seen_nobits && elf.section_data[jt]->size() != 0)
        throw std::logic_error("calculate_layout: a segment has file data after a NOBITS section");
      else if (elf.section_headers[jt].sh_addralign > constraint.alignment)
        throw std::logic_error("calculate_layout: a segment is less aligned than one of its sections");

      if (owner[jt] == NO_OWNER) owner[jt] = it;
      else already_claimed++;
      seen_nobits |= elf.section_headers[jt].sh_type == bits::to_underlying(SectionTypes::SHT_NOBITS);
    }
    // If non-0, this segment overlaps with a previous one. If all segments are claimed, this segment is a no-op for
    // placement. Partial overlap is complicated to compute addresses for, and as of 2026-09-06, unimplemented.
    // TODO: handle partial overlap cases without throwing, if possible.
    if (already_claimed != 0 && already_claimed != size_t(constraint.to_sec - constraint.from_sec + 1))
      throw std::logic_error("calculate_layout: segments partially overlap");
  }

  // Finalize header fields for program header table, excluding memory addresses.
  // Memory addresses will be assigned while evaluating section constraints.
  for (size_t i = 0; i < elf.section_headers.size(); ++i) {
    auto &shdr = elf.section_headers[i];
    // Per TIS ELF 1.2, every field of the null section entry is zero and it occupies no file space.
    if (i == 0) {
      shdr.sh_offset = shdr.sh_size = 0;
      continue;
    }
    // For pre-computed addresses, we need to perform additional work to ensure that the first section fulfills the
    // segments alignment requirement.
    else if (const auto claim = owner[i]; claim != NO_OWNER && !(*constraints)[claim].update_sec_addrs) {
      const auto &constraint = (*constraints)[claim];
      // Pre-compute addresses should match their requested alignment.
      if (const u64 own = std::max<u64>(shdr.sh_addralign, 1); shdr.sh_addr % own != 0)
        throw std::logic_error("calculate_layout: a section's address does not respect its sh_addralign");
      // Compute target offset to fulfill sh_offset % page_size == p_vaddr % page_size.
      if (i == constraint.from_sec) {
        const u64 align = std::max<u64>(constraint.alignment, 1);
        u64 congruent = bits::align_down(rolling_offset, align) + (shdr.sh_addr % align);
        if (congruent < rolling_offset) congruent += align;
        rolling_offset = congruent;
      }
      // Advance our offset by the difference in section addresses to match the caller's address assignment.
      else {
        const auto &anchor = elf.section_headers[constraint.from_sec];
        const u64 target = anchor.sh_offset + (shdr.sh_addr - anchor.sh_addr);
        if (shdr.sh_addr < anchor.sh_addr) throw std::logic_error("calculate_layout: section addresses must increase");
        else if (target < rolling_offset) throw std::logic_error("calculate_layout: a segment's sections overlap");
        else rolling_offset = target;
      }
    }
    // This function is responsible for computing the address of the section; ensure we abide by its alignment.
    else if (const u64 align = std::max<u64>(shdr.sh_addralign, over_align[i]); align > 1)
      rolling_offset = bits::align_up(rolling_offset, align);

    shdr.sh_offset = rolling_offset;
    // Per TIS ELF 1.2 on sh_size, sh_size is usually filesize, except for NOBITS where it is memory size.
    // Our storage reports file size, so we need a special case.
    if (shdr.sh_type != bits::to_underlying(SectionTypes::SHT_NOBITS))
      shdr.sh_size = elf.section_data[i]->size();
    rolling_offset = elf.section_data[i]->calculate_layout(ret, shdr.sh_offset);
  }

  // Apply segment layout constraints if provided
  for (u16 it = 0; constraints && it < constraints->size(); ++it) {
    const auto &constraint = (*constraints)[it];
    if (constraint.from_sec == 0) continue; // The caller manually edited this segment's header, so leave it alone.

    auto &phdr = elf.program_headers[it];
    phdr.p_offset = elf.section_headers[constraint.from_sec].sh_offset;
    // Do not use sh_size, because it reports memory size for NOBITS. Ask the underlying storage for actual size.
    phdr.p_filesz = elf.section_headers[constraint.to_sec].sh_offset + elf.section_data[constraint.to_sec]->size() -
                    elf.section_headers[constraint.from_sec].sh_offset;
    phdr.p_align = constraint.alignment;

    if (constraint.update_sec_addrs) {
      u64 base_address = bits::align_up(constraint.base_address, constraint.alignment);
      for (u16 jt = constraint.from_sec; jt <= constraint.to_sec; ++jt) {
        // Wholly covered by an earlier segment, which already placed every one of these sections.
        // TODO: will no longer be true when we allow partial overlap.
        if (owner[jt] != it) break;
        auto &shdr = elf.section_headers[jt];
        // Ensure that each section's address retains its requested alignment.
        if (shdr.sh_addralign > 1) base_address = bits::align_up(base_address, shdr.sh_addralign);
        shdr.sh_addr = base_address;
        base_address += shdr.sh_size;
      }
    }
    // Read the address back from the first section to avoid errors around pre-computed addresses, nested segments.
    phdr.p_vaddr = phdr.p_paddr = elf.section_headers[constraint.from_sec].sh_addr;

    // Compute last address of final section.
    const u64 end_to = elf.section_headers[constraint.to_sec].sh_addr + elf.section_headers[constraint.to_sec].sh_size;
    phdr.p_memsz = end_to - phdr.p_vaddr;
  }

  return ret;
}

} // namespace pepp::bts
