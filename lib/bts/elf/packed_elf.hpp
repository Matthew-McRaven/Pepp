#pragma once

#include "bts/bitmanip/copy.hpp"
#include "bts/elf/packed_storage.hpp"
#include "bts/elf/packed_types.hpp"
namespace pepp::bts {

template <ElfBits B, ElfEndian E> class PackedElf {
public:
  using Ehdr = PackedElfEhdr<B, E>;
  using Shdr = PackedElfShdr<B, E>;
  using Symbol = PackedElfSymbol<B, E>;
  using Phdr = PackedElfPhdr<B, E>;

  // Create an empty ELF file with the given file type and ABI
  PackedElf(ElfFileType, ElfMachineType, ElfABI);

  u32 add_section(Shdr &&shdr);
  u32 add_segment(Phdr &&phdr);
  u32 add_segment(SegmentType type, SegmentFlags flags = SegmentFlags::PF_NONE);

  Ehdr header;
  std::vector<Shdr> section_headers;
  std::vector<Phdr> program_headers;
  std::vector<std::shared_ptr<AStorage>> section_data;
};

using PackedElfLE32 = PackedElf<ElfBits::b32, ElfEndian::le>;
using PackedElfLE64 = PackedElf<ElfBits::b64, ElfEndian::le>;
using PackedElfBE32 = PackedElf<ElfBits::b32, ElfEndian::be>;
using PackedElfBE64 = PackedElf<ElfBits::b64, ElfEndian::be>;

template <ElfBits B, ElfEndian E>
pepp::bts::PackedElf<B, E>::PackedElf(ElfFileType type, ElfMachineType machine, ElfABI abi)
    : header(type, machine, abi) {}

template <ElfBits B, ElfEndian E> u32 PackedElf<B, E>::add_section(Shdr &&shdr) {
  if (section_headers.empty()) header.e_shentsize = sizeof(Shdr);
  section_headers.emplace_back(shdr);
  switch ((SectionTypes)(u32)shdr.sh_type) {
  default: section_data.emplace_back(std::make_shared<BlockStorage>()); break;
  }
  u32 ret = static_cast<u32>(section_headers.size() - 1);
  header.e_shnum = section_headers.size();
  return ret;
}

template <ElfBits B, ElfEndian E> u32 PackedElf<B, E>::add_segment(Phdr &&phdr) {
  if (program_headers.empty()) header.e_phentsize = sizeof(Phdr);
  program_headers.emplace_back(phdr);
  u32 ret = static_cast<u32>(program_headers.size() - 1);
  header.e_phnum = program_headers.size();
  return ret;
}

template <ElfBits B, ElfEndian E> u32 PackedElf<B, E>::add_segment(SegmentType type, SegmentFlags flags) {
  Phdr phdr;
  phdr.p_type = to_underlying(type);
  phdr.p_flags = to_underlying(flags);
  return add_segment(std::move(phdr));
}
} // namespace pepp::bts
