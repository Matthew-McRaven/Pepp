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
#include "core/ds/opaque_handle.hpp"
#include "core/formats/elf/enums.hpp"

namespace pepp::bts {

// Big enough to store both a ELF32 word abd ELF64 word.
// Needs to be masked / truncated when serializing to ELF32.
using uxword = u64;
using sxword = i64;

// Opaque handle to a section that is specifically not a section index. As part of the serialization process we
// construct a mapping of SectionRefs to actual indices.
struct SectionRefTag;
using SectionRef = OpaqueHandle<SectionRefTag>;

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

private:
  ElfBits _bits;
  ElfEndian _endian;
};

} // namespace pepp::bts
