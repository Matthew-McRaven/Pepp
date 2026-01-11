#pragma once

#include "bts/bitmanip/integers.h"
#include "bts/elf/types.hpp"

namespace pepp::bts {
enum class SectionIndices : u32 {
  SHN_UNDEF = 0,
  SHN_LORESERVE = 0xFF00,
  SHN_LOPROC = 0xFF00,
  SHN_HIPROC = 0xFF1F,
  SHN_LOOS = 0xFF20,
  SHN_HIOS = 0xFF3F,
  SHN_ABS = 0xFFF1,
  SHN_COMMON = 0xFFF2,
  SHN_XINDEX = 0xFFFF,
  SHN_HIRESERVE = 0xFFFF
};

enum class SectionTypes : u32 {
  SHT_NULL = 0,
  SHT_PROGBITS = 1,
  SHT_SYMTAB = 2,
  SHT_STRTAB = 3,
  SHT_RELA = 4,
  SHT_HASH = 5,
  SHT_DYNAMIC = 6,
  SHT_NOTE = 7,
  SHT_NOBITS = 8,
  SHT_REL = 9,
  SHT_SHLIB = 10,
  SHT_DYNSYM = 11,
  SHT_INIT_ARRAY = 14,
  SHT_FINI_ARRAY = 15,
  SHT_PREINIT_ARRAY = 16,
  SHT_GROUP = 17,
  SHT_SYMTAB_SHNDX = 18,
  SHT_GNU_ATTRIBUTES = 0x6ffffff5,
  SHT_GNU_HASH = 0x6ffffff6,
  SHT_GNU_LIBLIST = 0x6ffffff7,
  SHT_CHECKSUM = 0x6ffffff8,
  SHT_LOSUNW = 0x6ffffffa,
  SHT_SUNW_move = 0x6ffffffa,
  SHT_SUNW_COMDAT = 0x6ffffffb,
  SHT_SUNW_syminfo = 0x6ffffffc,
  SHT_GNU_verdef = 0x6ffffffd,
  SHT_GNU_verneed = 0x6ffffffe,
  SHT_GNU_versym = 0x6fffffff,
  SHT_LOOS = 0x60000000,
  SHT_HIOS = 0x6fffffff,
  SHT_LOPROC = 0x70000000,
  SHT_ARM_EXIDX = 0x70000001,
  SHT_ARM_PREEMPTMAP = 0x70000002,
  SHT_ARM_ATTRIBUTES = 0x70000003,
  SHT_ARM_DEBUGOVERLAY = 0x70000004,
  SHT_ARM_OVERLAYSECTION = 0x70000005,
  SHT_HIPROC = 0x7FFFFFFF,
  SHT_LOUSER = 0x80000000,
  SHT_HIUSER = 0xFFFFFFFF
};

// Can fit in 32 bits for 32bit targets, but use wider u64 to accomodate 64-bit targets.
enum class SectionFlags : u64 {
  SHF_WRITE = 0x1,
  SHF_ALLOC = 0x2,
  SHF_EXECINSTR = 0x4,
  SHF_MERGE = 0x10,
  SHF_STRINGS = 0x20,
  SHF_INFO_LINK = 0x40,
  SHF_LINK_ORDER = 0x80,
  SHF_OS_NONCONFORMING = 0x100,
  SHF_GROUP = 0x200,
  SHF_TLS = 0x400,
  SHF_COMPRESSED = 0x800,
  SHF_GNU_RETAIN = 0x200000,
  SHF_GNU_MBIND = 0x01000000,
  SHF_MASKOS = 0x0FF00000,
  SHF_MIPS_GPREL = 0x10000000,
  SHF_ORDERED = 0x40000000,
  SHF_EXCLUDE = 0x80000000,
  SHF_MASKPROC = 0xF0000000
};

#pragma pack(push, 1)
template <ElfBits B, ElfEndian E> struct ElfShdr {
  U32<E> sh_name;
  U32<E> sh_type;
  Word<B, E> sh_flags;
  Word<B, E> sh_addr;
  Word<B, E> sh_offset;
  Word<B, E> sh_size;
  U32<E> sh_link;
  U32<E> sh_info;
  Word<B, E> sh_addralign;
  Word<B, E> sh_entsize;
};
#pragma pack(pop)

template <ElfBits B, ElfEndian E> ElfShdr<B, E> create_null_header() {
  ElfShdr<B, E> shdr;
  shdr.sh_name = 0;
  shdr.sh_type = to_underlying(SectionTypes::SHT_NULL);
  shdr.sh_flags = 0;
  shdr.sh_addr = 0;
  shdr.sh_offset = 0;
  shdr.sh_size = 0;
  shdr.sh_link = to_underlying(SectionIndices::SHN_UNDEF);
  shdr.sh_info = 0;
  shdr.sh_addralign = 0;
  shdr.sh_entsize = 0;
  return shdr;
}
template <ElfBits B, ElfEndian E> ElfShdr<B, E> create_shstrtab_header(u32 name) {
  ElfShdr<B, E> shdr;
  shdr.sh_name = name;
  shdr.sh_type = to_underlying(SectionTypes::SHT_STRTAB);
  shdr.sh_flags = 0;
  shdr.sh_addr = 0;
  shdr.sh_offset = 0;
  shdr.sh_size = 0;
  shdr.sh_link = to_underlying(SectionIndices::SHN_UNDEF);
  shdr.sh_info = 0;
  shdr.sh_addralign = 1;
  shdr.sh_entsize = 0;
  return shdr;
}

using ElfShdrLE32 = ElfShdr<ElfBits::b32, ElfEndian::le>;
using ElfShdrLE64 = ElfShdr<ElfBits::b64, ElfEndian::le>;
using ElfShdrBE32 = ElfShdr<ElfBits::b32, ElfEndian::be>;
using ElfShdrBE64 = ElfShdr<ElfBits::b64, ElfEndian::be>;

} // namespace pepp::bts
