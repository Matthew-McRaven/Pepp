#pragma once

#include "bts/bitmanip/copy.hpp"
#include "bts/elf/packed_types.hpp"
namespace pepp::bts {
struct LayoutItem {
  u64 offset;
  std::span<u8> data;
};
template <ElfBits B, ElfEndian E> class PackedElf {
public:
  using Ehdr = PackedElfEhdr<B, E>;
  using Shdr = PackedElfShdr<B, E>;
  using Phdr = PackedElfPhdr<B, E>;
  // Create an empty ELF file with the given file type and ABI
  PackedElf(ElfFileType, ElfMachineType, ElfABI);

  void add_section_header_table();

  u32 add_section(Shdr &&shdr);
  u32 add_segment(Phdr &&phdr);
  // Place the section header followed by the program header table at the given offset
  u64 place_header_tables_at(std::vector<LayoutItem> &layout, u64 off);

  std::vector<LayoutItem> calculate_layout();
  void write(std::span<u8> out, const std::vector<LayoutItem> &layout);

  Ehdr header;
  std::vector<Shdr> section_headers;
  std::vector<Phdr> program_headers;
  std::vector<std::vector<u8>> section_data;

private:
};
using PackedElfLE32 = PackedElf<ElfBits::b32, ElfEndian::le>;
using PackedElfLE64 = PackedElf<ElfBits::b64, ElfEndian::le>;
using PackedElfBE32 = PackedElf<ElfBits::b32, ElfEndian::be>;
using PackedElfBE64 = PackedElf<ElfBits::b64, ElfEndian::be>;

template <ElfBits B, ElfEndian E>
pepp::bts::PackedElf<B, E>::PackedElf(ElfFileType type, ElfMachineType machine, ElfABI abi)
    : header(type, machine, abi) {}

template <ElfBits B, ElfEndian E> inline void PackedElf<B, E>::add_section_header_table() {
  if (!section_headers.empty()) return;
  add_section(create_null_header<B, E>());
  add_section(create_shstrtab_header<B, E>(1));

  header.e_shstrndx = 1;
  auto &shstrab = section_data.back();
  shstrab.push_back(0);
  shstrab.insert(shstrab.end(), {'.', 's', 'h', 's', 't', 'r', 't', 'a', 'b', 0});
}

template <ElfBits B, ElfEndian E> inline u32 PackedElf<B, E>::add_section(Shdr &&shdr) {
  if (section_headers.empty()) header.e_shentsize = sizeof(Shdr);
  section_headers.emplace_back(shdr);
  section_data.emplace_back();
  u32 ret = static_cast<u32>(section_headers.size() - 1);
  header.e_shnum = section_headers.size();
  return ret;
}

template <ElfBits B, ElfEndian E> inline u32 PackedElf<B, E>::add_segment(Phdr &&phdr) {
  if (program_headers.empty()) header.e_phentsize = sizeof(Phdr);
  program_headers.emplace_back(phdr);
  u32 ret = static_cast<u32>(program_headers.size() - 1);
  header.e_phnum = program_headers.size();
  return ret;
}

template <ElfBits B, ElfEndian E>
inline u64 PackedElf<B, E>::place_header_tables_at(std::vector<LayoutItem> &layout, u64 off) {
  // Then place section header table
  if (!section_headers.empty()) {
    header.e_shoff = off;
    layout.emplace_back(LayoutItem{
        off, std::span<u8>(reinterpret_cast<u8 *>(section_headers.data()), sizeof(Shdr) * section_headers.size())});
    off += header.e_shentsize * header.e_shnum;
  }
  // Followed by program header table
  if (!program_headers.empty()) {
    header.e_phoff = off;
    layout.emplace_back(LayoutItem{
        off, std::span<u8>(reinterpret_cast<u8 *>(program_headers.data()), sizeof(Phdr) * program_headers.size())});
    off += header.e_phentsize * header.e_phnum;
  }
  return off;
}

template <ElfBits B, ElfEndian E> inline std::vector<LayoutItem> PackedElf<B, E>::calculate_layout() {
  std::vector<LayoutItem> ret;
  // 3 is a magic constant including: ehdr, shdr, and phdr
  ret.reserve(section_headers.size() + 2);
  ret.emplace_back(LayoutItem{0, std::span<u8>(reinterpret_cast<u8 *>(&header), sizeof(Ehdr))});
  u64 rolling_offset = sizeof(Ehdr);
  rolling_offset = place_header_tables_at(ret, rolling_offset);
  // Finalize header fields for program header table
  // Skip first section (null), because we want a 0-offset.
  for (size_t i = 1; i < section_headers.size(); ++i) {
    auto &shdr = section_headers[i];
    shdr.sh_offset = rolling_offset;
    auto size = section_data[i].size();
    shdr.sh_size = size, rolling_offset += size;
    ret.emplace_back(LayoutItem{shdr.sh_offset, std::span<u8>(section_data[i].data(), size)});
  }

  return ret;
}

template <ElfBits B, ElfEndian E>
inline void PackedElf<B, E>::write(std::span<u8> out, const std::vector<LayoutItem> &layout) {
  for (const auto &item : layout) {
    if (item.offset + item.data.size() > out.size())
      throw std::runtime_error("Elf::write: layout item exceeds output size");
    std::span<u8> chunk = out.subspan(item.offset, item.data.size());
    bits::memcpy<u8, u8>(chunk, {item.data});
  }
}

u64 size_for_layout(const std::vector<pepp::bts::LayoutItem> &layout) {
  u64 ret = 0;
  for (const auto &item : layout) ret = std::max(ret, item.offset + item.data.size());
  return ret;
}

} // namespace pepp::bts
