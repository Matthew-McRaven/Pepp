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

#include "core/formats/elf/packed_storage.hpp"
#include "core/formats/elf/packed_types.hpp"
namespace pepp::bts {

// Packed POD class representing an ELF file in memory
// Not meant to be instantiated directly. If you want to create a RO instance from a file, use PackedInputElfFile.
// If you intend to create a new ELF file from scratch, use PackedGrowableElfFile.
template <ElfBits B, ElfEndian E> class PackedElf {
public:
  // The template parameters, so code holding an AnyPackedElfPtr can check a file's format at runtime.
  static constexpr ElfBits elf_bits = B;
  static constexpr ElfEndian elf_endian = E;
  using Ehdr = PackedElfEhdr<B, E>;
  using Shdr = PackedElfShdr<B, E>;
  using Symbol = PackedElfSymbol<B, E>;
  using Phdr = PackedElfPhdr<B, E>;

  Ehdr header;
  std::vector<Shdr> section_headers;
  std::vector<Phdr> program_headers;
  std::vector<std::shared_ptr<AStorage>> section_data;
};

// Helper classes to delegate between a lazy MappedFile and eagerly-loaded contiguous buffer.
struct AElfSource {
  virtual ~AElfSource() = 0;
  // Storage covering [offset, offset + length) of the image.
  virtual std::shared_ptr<AStorage> slice(u64 offset, u64 length) const = 0;
};

// Reads through a memory-mapped file, mapping each region on first access.
struct MappedFileSource final : public AElfSource {
  explicit MappedFileSource(std::shared_ptr<MappedFile> file);
  std::shared_ptr<AStorage> slice(u64 offset, u64 length) const override;

private:
  std::shared_ptr<MappedFile> _file;
};

// Re-use the serialized bytes of an ELF file that is already in memory.
struct BufferSource final : public AElfSource {
  explicit BufferSource(std::vector<char> &&bytes);
  std::shared_ptr<AStorage> slice(u64 offset, u64 length) const override;

private:
  std::shared_ptr<BlockStorage> _bytes;
};
// A packed ELF file that is read-only and backed by a lazily memory-mapped file or eager contiguous buffer.
// While its elf header, section headers, and program headers are always eagerly loaded into memory,
// section data will be lazily loaded on first use.
template <ElfBits B, ElfEndian E> class PackedInputElfFile : public PackedElf<B, E> {
  struct private_ctor_tag {};

public:
  // Overload which allows construction from either a memory-mapped file or contiguous buffer.
  PackedInputElfFile(std::shared_ptr<const AElfSource> source);
  // On construction, will read in ehdr, shdrs, and phdrs, throwing if the file's class or byte order is not B / E.
  // Section data will be loaded lazily.
  PackedInputElfFile(std::shared_ptr<MappedFile> file);
  PackedInputElfFile(std::string file);

  // Since this file is (usually) backed by a memory-mapped file, it's cheap to produce a non-owning slice over a given
  // segments data. The segment's bytes are loaded on first access, and the return value is cached across valls.
  std::shared_ptr<const AStorage> segment_data(u16 index) const;

private:
  std::shared_ptr<const AElfSource> _source;
  mutable std::vector<std::shared_ptr<AStorage>> _segment_data;
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

template <ElfBits B, ElfEndian E>
PackedInputElfFile<B, E>::PackedInputElfFile(std::shared_ptr<const AElfSource> source) : _source(std::move(source)) {
  if (!_source) throw std::invalid_argument("PackedInputElfFile: source must be non-null");
  // Read in all header data and eagerly copy it to our packed structures
  auto header_slice = _source->slice(0, sizeof(typename PackedElf<B, E>::Ehdr));
  auto header_data = header_slice->get(0, header_slice->size());
  if (header_data.size() < sizeof(typename PackedElf<B, E>::Ehdr))
    throw std::runtime_error("File too small to contain ELF header");
  auto header_dest = bits::span<u8>((u8 *)&this->header, sizeof(typename PackedElf<B, E>::Ehdr));
  std::memcpy(header_dest.data(), header_data.data(), header_dest.size());
  // Every later offset is read as a B-sized, E-ordered field, so reject other formats before trusting them.
  constexpr auto elf_class = B == ElfBits::b32 ? ElfClass::ELFCLASS32 : ElfClass::ELFCLASS64;
  constexpr auto elf_data = E == ElfEndian::le ? ElfEncoding::ELFDATA2LSB : ElfEncoding::ELFDATA2MSB;
  if (!this->header.has_valid_magic()) throw std::runtime_error("File is not an ELF file");
  else if (this->header.ei_class() != elf_class || this->header.ei_data() != elf_data)
    throw std::runtime_error("ELF file has the wrong class or byte order");
  // Determine base address of section header table and eagerly copy into our shdr vector.
  word<B> shdr_start = this->header.e_shoff, shdr_size = this->header.e_shentsize * this->header.e_shnum;
  if (shdr_size > 0) {
    auto shdr_slice = _source->slice(shdr_start, shdr_size);
    auto shdr_data = shdr_slice->get(0, shdr_slice->size());
    if (shdr_data.size() < shdr_size) throw std::runtime_error("File too small to contain section header table");
    this->section_headers.resize(this->header.e_shnum);
    auto shdr_dest = bits::span<u8>(reinterpret_cast<u8 *>(this->section_headers.data()), shdr_size);
    std::memcpy(shdr_dest.data(), shdr_data.data(), shdr_dest.size());
  }
  // Determine base address of program header table and eagerly copy into our phdr vector.
  word<B> phdr_start = this->header.e_phoff, phdr_size = this->header.e_phentsize * this->header.e_phnum;
  if (phdr_size > 0) {
    auto phdr_slice = _source->slice(phdr_start, phdr_size);
    auto phdr_data = phdr_slice->get(0, phdr_slice->size());
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
    // NOBITS owns no file bytes; its sh_size is a memory size, and the bytes at its sh_offset belong to someone else.
    if (shdr.sh_type == bits::to_underlying(SectionTypes::SHT_NOBITS))
      this->section_data[it] = std::make_shared<NullStorage>();
    else this->section_data[it] = _source->slice(shdr.sh_offset, shdr.sh_size);
  }
}

template <ElfBits B, ElfEndian E>
PackedInputElfFile<B, E>::PackedInputElfFile(std::shared_ptr<MappedFile> file)
    : PackedInputElfFile(std::make_shared<MappedFileSource>(std::move(file))) {}

template <ElfBits B, ElfEndian E>
PackedInputElfFile<B, E>::PackedInputElfFile(std::string file) : PackedInputElfFile(MappedFile::open_readonly(file)) {}

template <ElfBits B, ElfEndian E>
std::shared_ptr<const AStorage> PackedInputElfFile<B, E>::segment_data(u16 index) const {
  if (index >= this->program_headers.size()) throw std::out_of_range("segment_data: segment index out of range");
  if (_segment_data.size() != this->program_headers.size()) _segment_data.resize(this->program_headers.size());
  if (_segment_data[index] != nullptr) return _segment_data[index];

  const auto &phdr = this->program_headers[index];
  return _segment_data[index] = _source->slice(phdr.p_offset, phdr.p_filesz);
}

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
  phdr.p_type = bits::to_underlying(type);
  phdr.p_flags = bits::to_underlying(flags);
  return add_segment(std::move(phdr));
}

} // namespace pepp::bts
