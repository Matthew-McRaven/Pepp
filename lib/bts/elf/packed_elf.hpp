#pragma once

#include "bts/bitmanip/copy.hpp"
#include "bts/elf/packed_storage.hpp"
#include "bts/elf/packed_types.hpp"
namespace pepp::bts {

// Packed POD class representing an ELF file in memory
// Not meant to be instantiated directly. If you want to create a RO instance from a file, use PackedInputElfFile.
// If you intend to create a new ELF file from scratch, use PackedGrowableElfFile.
template <ElfBits B, ElfEndian E> class PackedElf {
public:
  using Ehdr = PackedElfEhdr<B, E>;
  using Shdr = PackedElfShdr<B, E>;
  using Symbol = PackedElfSymbol<B, E>;
  using Phdr = PackedElfPhdr<B, E>;

  Ehdr header;
  std::vector<Shdr> section_headers;
  std::vector<Phdr> program_headers;
  std::vector<std::shared_ptr<AStorage>> section_data;
};

using PackedElfLE32 = PackedElf<ElfBits::b32, ElfEndian::le>;
static_assert(std::is_standard_layout_v<PackedElfLE32>);
using PackedElfBE32 = PackedElf<ElfBits::b32, ElfEndian::be>;
static_assert(std::is_standard_layout_v<PackedElfBE32>);
using PackedElfLE64 = PackedElf<ElfBits::b64, ElfEndian::le>;
static_assert(std::is_standard_layout_v<PackedElfLE64>);
using PackedElfBE64 = PackedElf<ElfBits::b64, ElfEndian::be>;
static_assert(std::is_standard_layout_v<PackedElfBE64>);

template <ElfBits B, ElfEndian E> class PackedInputElfFile : public PackedElf<B, E> {
public:
};

template <ElfBits B, ElfEndian E> class PackedGrowableElfFile : public PackedElf<B, E> {
public:
  using Ehdr = PackedElfEhdr<B, E>;
  using Shdr = PackedElfShdr<B, E>;
  using Symbol = PackedElfSymbol<B, E>;
  using Phdr = PackedElfPhdr<B, E>;

  // Create an empty ELF file with the given file type and ABI
  PackedGrowableElfFile(ElfFileType, ElfMachineType, ElfABI);

  u32 add_section(Shdr &&shdr);
  u32 add_segment(Phdr &&phdr);
  u32 add_segment(SegmentType type, SegmentFlags flags = SegmentFlags::PF_NONE);
};

using PackedGrowableElfLE32 = PackedGrowableElfFile<ElfBits::b32, ElfEndian::le>;
using PackedGrowableElfBE32 = PackedGrowableElfFile<ElfBits::b32, ElfEndian::be>;
using PackedGrowableElfLE64 = PackedGrowableElfFile<ElfBits::b64, ElfEndian::le>;
using PackedGrowableElfBE64 = PackedGrowableElfFile<ElfBits::b64, ElfEndian::be>;

template <ElfBits B, ElfEndian E>
pepp::bts::PackedGrowableElfFile<B, E>::PackedGrowableElfFile(ElfFileType type, ElfMachineType machine, ElfABI abi) {
  this->header = std::move(Ehdr(type, machine, abi));
}

template <ElfBits B, ElfEndian E> u32 PackedGrowableElfFile<B, E>::add_section(Shdr &&shdr) {
  if (this->section_headers.empty()) this->header.e_shentsize = sizeof(Shdr);
  this->section_headers.emplace_back(shdr);
  switch ((SectionTypes)(u32)shdr.sh_type) {
  case SectionTypes::SHT_STRTAB: this->section_data.emplace_back(std::make_shared<PagedStorage>()); break;
  default: this->section_data.emplace_back(std::make_shared<BlockStorage>()); break;
  }
  u32 ret = static_cast<u32>(this->section_headers.size() - 1);
  this->header.e_shnum = this->section_headers.size();
  return ret;
}

template <ElfBits B, ElfEndian E> u32 PackedGrowableElfFile<B, E>::add_segment(Phdr &&phdr) {
  if (this->program_headers.empty()) this->header.e_phentsize = sizeof(Phdr);
  this->program_headers.emplace_back(phdr);
  u32 ret = static_cast<u32>(this->program_headers.size() - 1);
  this->header.e_phnum = this->program_headers.size();
  return ret;
}

template <ElfBits B, ElfEndian E> u32 PackedGrowableElfFile<B, E>::add_segment(SegmentType type, SegmentFlags flags) {
  Phdr phdr;
  phdr.p_type = to_underlying(type);
  phdr.p_flags = to_underlying(flags);
  return add_segment(std::move(phdr));
}
} // namespace pepp::bts
