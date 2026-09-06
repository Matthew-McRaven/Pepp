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
#include <memory>
#include <vector>
#include "core/formats/elf/enums.hpp"
#include "core/formats/elf/managed_types.hpp"

namespace pepp::bts {

class ManagedSection;
class ManagedSegment;

/*
 * An ELF file held as a series of in-memory, editable data structures rather than as packed bytes.
 * It is designed for constructing new ELF files from scratch (e.g., assembler outputs) rather than editing existing
 * object code. Data is usually kept in host-endian format and only converted to target endianness on serialization.
 *
 * Because this is an abstraction to make it easier to incrmeentally construct an ELF file, some header fields are not
 * stored and are instead computed as-needed The following fields are explictly not stored:
 *   e_ident[EI_MAG*], [EI_VERSION], [EI_PAD]  constants
 *   e_version                                 always EV_CURRENT
 *   e_ehsize, e_phentsize, e_shentsize        depends only on _bits
 *   e_phnum, e_shnum                          segment / section counts
 *   e_phoff, e_shoff                          assigned during serialization
 *   e_shstrndx                                assigned when .shstrtab is created + serialized
 */
class ManagedElf {
public:
  ManagedElf(ElfBits bits, ElfEndian endian, ElfFileType type, ElfMachineType machine,
             ElfABI abi = ElfABI::ELFOSABI_NONE);
  ~ManagedElf();
  ManagedElf(const ManagedElf &) = delete;
  ManagedElf(ManagedElf &&) noexcept;
  ManagedElf &operator=(ManagedElf &&) noexcept;

  // Target format for serialization. Read-only to avoid having to swap endianness and size after construction.
  ElfBits bits() const noexcept { return _bits; }
  ElfEndian endian() const noexcept { return _endian; }

  ElfFileType type;
  ElfMachineType machine;
  ElfABI abi = ElfABI::ELFOSABI_NONE;
  u8 abi_version = 0;
  // Processor-specific flags; RISC-V uses these (EF_RISCV_RVC, float ABI, ...).
  u32 flags = 0;
  // TODO: once symbols exist, swap to variant<uxword, SymbolRef> so an entry point can be
  // a named value rather than an address.
  uxword entry = 0;

  // Pesudeo-sections that exist to avoid dealing with section indices.
  // If you change this list, you must also update CTOR and last_magic_section().
  static constexpr SectionRef SHN_UNDEF = SectionRef{0}, SHN_ABS = SectionRef{1}, SHN_COMMON = SectionRef{2};

  // The section holding section names, which translates to e_shstrndx.
  SectionRef shstrtab() const noexcept { return _shstrtab; }
  void set_shstrtab(SectionRef ref) noexcept { _shstrtab = ref; }

  std::size_t section_count() const noexcept { return _sections.empty() ? 0 : _sections.size() - 1; }
  SectionRef add_section(std::string name, SectionTypes type);
  // Null for both default-constructed (SHN_UNDEF) and out-of-range handle.
  ManagedSection *section(SectionRef ref) noexcept;
  const ManagedSection *section(SectionRef ref) const noexcept;
  // Highest used SectionRef to aid iteration over sections.
  SectionRef last_section() const noexcept;
  // Includes initial null section and pseudo-sections.
  std::span<const std::unique_ptr<ManagedSection>> sections() const noexcept;

  std::size_t segment_count() const noexcept { return _segments.empty() ? 0 : _segments.size() - 1; }
  SegmentRef add_segment(SegmentType type, SegmentFlags flags = {});
  ManagedSegment *segment(SegmentRef ref) noexcept;
  const ManagedSegment *segment(SegmentRef ref) const noexcept;
  // Highest used SegmentRef to aid iteration over segments.
  SegmentRef last_segment() const noexcept;
  // Includes initial null segment.
  std::span<const std::unique_ptr<ManagedSegment>> segments() const noexcept;

private:
  ElfBits _bits;
  ElfEndian _endian;
  SectionRef last_magic_section() const noexcept;
  SectionRef _shstrtab = SHN_UNDEF;
  // Indexed by SectionRef::value. Once handed out, that SectionRef must be valid for the lifetime of this class.
  // Must use extra level of indirection via unique_ptr to gaurentee pointer stability.
  std::vector<std::unique_ptr<ManagedSection>> _sections;
  // as with _sections, 0 is unused so a default-constructed SegmentRef points to a null segment.
  std::vector<std::unique_ptr<ManagedSegment>> _segments;
};

} // namespace pepp::bts
