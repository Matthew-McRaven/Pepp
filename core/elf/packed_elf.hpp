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

#include "core/elf/packed_storage.hpp"
#include "core/elf/packed_types.hpp"
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
static_assert(std::is_standard_layout_v<PackedElfLE32>);
static_assert(std::is_standard_layout_v<PackedElfBE32>);
static_assert(std::is_standard_layout_v<PackedElfLE64>);
static_assert(std::is_standard_layout_v<PackedElfBE64>);

// A packed ELF file that is read-only and backed by a memory-mapped file.
// While its elf header, section headers, and program headers are eagerly loaded into memory,
// section data will be lazily loaded on first use.
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

// A packed ELF file that can be modified and grown (relatively) inexpensively.
// While being modified, it is still stored in memory in the proper packed disk format (minus proper sh_offsets)
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
  // Read in all header data and eagerly copy it to our packed structures
  auto header_slice = file->slice(0, sizeof(typename PackedElf<B, E>::Ehdr));
  auto header_data = header_slice->get();
  if (header_data.size() < sizeof(typename PackedElf<B, E>::Ehdr))
    throw std::runtime_error("File too small to contain ELF header");
  auto header_dest = bits::span<u8>((u8 *)&this->header, sizeof(typename PackedElf<B, E>::Ehdr));
  std::memcpy(header_dest.data(), header_data.data(), header_dest.size());
  // Determine base address of section header table and eagerly copy into our shdr vector.
  word<B> shdr_start = this->header.e_shoff, shdr_size = this->header.e_shentsize * this->header.e_shnum;
  if (shdr_size > 0) {
    auto shdr_slice = file->slice(shdr_start, shdr_size);
    auto shdr_data = shdr_slice->get();
    if (shdr_data.size() < shdr_size) throw std::runtime_error("File too small to contain section header table");
    this->section_headers.resize(this->header.e_shnum);
    auto shdr_dest = bits::span<u8>(reinterpret_cast<u8 *>(this->section_headers.data()), shdr_size);
    std::memcpy(shdr_dest.data(), shdr_data.data(), shdr_dest.size());
  }
  // Determine base address of program header table and eagerly copy into our phdr vector.
  word<B> phdr_start = this->header.e_phoff, phdr_size = this->header.e_phentsize * this->header.e_phnum;
  if (phdr_size > 0) {
    auto phdr_slice = file->slice(phdr_start, phdr_size);
    auto phdr_data = phdr_slice->get();
    if (phdr_data.size() < phdr_size) throw std::runtime_error("File too small to contain program header table");
    this->program_headers.resize(this->header.e_phnum);
    auto phdr_dest = bits::span<u8>(reinterpret_cast<u8 *>(this->program_headers.data()), phdr_size);
    std::memcpy(phdr_dest.data(), phdr_data.data(), phdr_dest.size());
  }
  // Ensure section_data matches size of section_headers.
  this->section_data.resize(this->section_headers.size(), nullptr);
  if (this->section_headers.empty()) return;
  this->section_data[0] = std::make_shared<NullStorage>(); // Section 0 is always SHT_NULL
  // Construct an AStorage which will lazily map in each section on demand.
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
  case SectionTypes::SHT_NULL: [[fallthrough]];
  case SectionTypes::SHT_NOBITS: this->section_data.emplace_back(std::make_shared<NullStorage>()); break;
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

} // namespace pepp::core
