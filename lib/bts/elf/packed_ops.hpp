#pragma once

#include "bts/elf/packed_elf.hpp"
#include "bts/elf/packed_types.hpp"
namespace pepp::bts {

template <ElfBits B, ElfEndian E> void ensure_section_header_table(PackedElf<B, E> &elf);
template <ElfBits B, ElfEndian E> u64 place_header_tables_at(PackedElf<B, E> &, std::vector<LayoutItem> &, u64 off);
template <ElfBits B, ElfEndian E> std::vector<LayoutItem> calculate_layout(PackedElf<B, E> &);
u64 size_for_layout(const std::vector<pepp::bts::LayoutItem> &layout) noexcept;
void write(std::span<u8> out, const std::vector<LayoutItem> &layout);

template <ElfBits B, ElfEndian E> void ensure_section_header_table(PackedElf<B, E> &elf) {
  if (!elf.section_headers.empty()) return;
  elf.add_section(create_null_header<B, E>());
  auto idx = elf.add_section(create_shstrtab_header<B, E>(1));

  elf.header.e_shstrndx = 1;
  auto &shstrab = elf.section_data.back();
  shstrab.push_back(0);
  shstrab.insert(shstrab.end(), {'.', 's', 'h', 's', 't', 'r', 't', 'a', 'b', 0});
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

template <ElfBits B, ElfEndian E> std::vector<LayoutItem> calculate_layout(PackedElf<B, E> &elf) {
  using PackedElf = PackedElf<B, E>;
  std::vector<LayoutItem> ret;
  // 3 is a magic constant including: ehdr, shdr, and phdr
  ret.reserve(elf.section_headers.size() + 2);
  ret.emplace_back(LayoutItem{0, std::span<u8>(reinterpret_cast<u8 *>(&elf.header), sizeof(typename PackedElf::Ehdr))});
  u64 rolling_offset = sizeof(typename PackedElf::Ehdr);
  rolling_offset = place_header_tables_at(elf, ret, rolling_offset);
  // Finalize header fields for program header table
  // Skip first section (null), because we want a 0-offset.
  for (size_t i = 1; i < elf.section_headers.size(); ++i) {
    auto &shdr = elf.section_headers[i];
    shdr.sh_offset = rolling_offset;
    auto size = elf.section_data[i].size();
    shdr.sh_size = size, rolling_offset += size;
    ret.emplace_back(LayoutItem{shdr.sh_offset, std::span<u8>(elf.section_data[i].data(), size)});
  }

  return ret;
}

} // namespace pepp::bts
