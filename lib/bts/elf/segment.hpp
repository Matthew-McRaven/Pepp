#pragma once

#include "bts/elf/types.hpp"
namespace pepp::bts {

enum class SegmentType : u32 {
  PT_NULL = 0,
  PT_LOAD = 1,
  PT_DYNAMIC = 2,
  PT_INTERP = 3,
  PT_NOTE = 4,
  PT_SHLIB = 5,
  PT_PHDR = 6,
  PT_TLS = 7,
  PT_GNU_EH_FRAME = 0x6474e550,
  PT_GNU_STACK = 0x6474e551,
  PT_GNU_RELRO = 0x6474e552,
  PT_GNU_PROPERTY = 0x6474e553,
  PT_OPENBSD_RANDOMIZE = 0x65a3dbe6,
  PT_ARM_EXIDX = 0x70000001,
  PT_RISCV_ATTRIBUTES = 0x70000003,
};

enum class SegmentFlags : u32 {
  PF_NONE = 0,
  PF_X = 1,
  PF_W = 2,
  PF_R = 4,
};

template <ElfBits B, ElfEndian E> struct ElfPhdr;
template <ElfEndian E> struct ElfPhdr<ElfBits::b32, E> {
  U32<E> p_type;
  U32<E> p_offset;
  U32<E> p_vaddr;
  U32<E> p_paddr;
  U32<E> p_filesz;
  U32<E> p_memsz;
  U32<E> p_flags;
  U32<E> p_align;
};

template <ElfEndian E> struct ElfPhdr<ElfBits::b64, E> {
  U32<E> p_type;
  U32<E> p_flags;
  U64<E> p_offset;
  U64<E> p_vaddr;
  U64<E> p_paddr;
  U64<E> p_filesz;
  U64<E> p_memsz;
  U64<E> p_align;
};

using ElfPhdrLE32 = ElfPhdr<ElfBits::b32, ElfEndian::le>;
using ElfPhdrLE64 = ElfPhdr<ElfBits::b64, ElfEndian::le>;
using ElfPhdrBE32 = ElfPhdr<ElfBits::b32, ElfEndian::be>;
using ElfPhdrBE64 = ElfPhdr<ElfBits::b64, ElfEndian::be>;
} // namespace pepp::bts
