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
  struct private_ctor_tag {};

public:
  // Create a read-only ELF file from a memory-mapped file.
  // On construction, will read in ehdr, shdrs, and phdrs.
  // Section data will be loaded lazily.
  PackedInputElfFile(std::shared_ptr<MappedFile> file);
  PackedInputElfFile(std::string file);

private:
  std::shared_ptr<MappedFile> _file;
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
PackedInputElfFile<B, E>::PackedInputElfFile(std::shared_ptr<MappedFile> file) : _file(file) {
  auto header_slice = file->slice(0, sizeof(typename PackedElf<B, E>::Ehdr));
  auto header_data = header_slice->get();
  if (header_data.size() < sizeof(typename PackedElf<B, E>::Ehdr))
    throw std::runtime_error("File too small to contain ELF header");
  auto header_dest = bits::span<u8>((u8 *)&this->header, sizeof(typename PackedElf<B, E>::Ehdr));
  std::memcpy(header_dest.data(), header_data.data(), header_dest.size());
  word<B> shdr_start = this->header.e_shoff, shdr_size = this->header.e_shentsize * this->header.e_shnum;
  if (shdr_size > 0) {
    auto shdr_slice = file->slice(shdr_start, shdr_size);
    auto shdr_data = shdr_slice->get();
    if (shdr_data.size() < shdr_size) throw std::runtime_error("File too small to contain section header table");
    this->section_headers.resize(this->header.e_shnum);
    auto shdr_dest = bits::span<u8>(reinterpret_cast<u8 *>(this->section_headers.data()), shdr_size);
    std::memcpy(shdr_dest.data(), shdr_data.data(), shdr_dest.size());
  }
  word<B> phdr_start = this->header.e_phoff, phdr_size = this->header.e_phentsize * this->header.e_phnum;
  if (phdr_size > 0) {
    auto phdr_slice = file->slice(phdr_start, phdr_size);
    auto phdr_data = phdr_slice->get();
    if (phdr_data.size() < phdr_size) throw std::runtime_error("File too small to contain program header table");
    this->program_headers.resize(this->header.e_phnum);
    auto phdr_dest = bits::span<u8>(reinterpret_cast<u8 *>(this->program_headers.data()), phdr_size);
    std::memcpy(phdr_dest.data(), phdr_data.data(), phdr_dest.size());
  }
  this->section_data.resize(this->section_headers.size(), nullptr);
  if (this->section_headers.empty()) return;
  this->section_data[0] = std::make_shared<BlockStorage>(); // Null section
  for (int it = 1; it < this->section_headers.size(); ++it) {
    const auto &shdr = this->section_headers[it];
    this->section_data[it] = std::make_shared<MemoryMapped>(file->slice(shdr.sh_offset, shdr.sh_size));
  }
}

template <ElfBits B, ElfEndian E>
PackedInputElfFile<B, E>::PackedInputElfFile(std::string file) : PackedInputElfFile(MappedFile::open_readonly(file)) {}

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
