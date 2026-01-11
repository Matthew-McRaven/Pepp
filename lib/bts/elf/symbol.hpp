#pragma once

#include "./section.hpp"
#include "bts/bitmanip/integers.h"
#include "bts/elf/types.hpp"
namespace pepp::bts {
enum class SymbolBinding : u8 {
  STB_LOCAL = 0,
  STB_GLOBAL = 1,
  STB_WEAK = 2,
  STB_LOOS = 10,
  STB_HIOS = 12,
  STB_MULTIDEF = 13,
  STB_LOPROC = 13,
  STB_HIPROC = 15
};

enum class SymbolType : u8 {
  // Symbol types
  STT_NOTYPE = 0,
  STT_OBJECT = 1,
  STT_FUNC = 2,
  STT_SECTION = 3,
  STT_FILE = 4,
  STT_COMMON = 5,
  STT_TLS = 6,
  STT_LOOS = 10,
  STT_HIOS = 12,
  STT_LOPROC = 13,
  STT_HIPROC = 15
};

enum class SymbolVisibility : u8 {
  STV_DEFAULT = 0,
  STV_INTERNAL = 1,
  STV_HIDDEN = 2,
  STV_PROTECTED = 3,
};

#pragma pack(push, 1)
template <ElfBits B, ElfEndian E> struct ElfSymbol {
  using enum SectionIndices;
  using enum SymbolBinding;
  bool is_undef() const { return st_shndx == to_underlying(SHN_UNDEF); }
  bool is_abs() const { return st_shndx == to_underlying(SHN_ABS); }
  bool is_common() const { return st_shndx == to_underlying(SHN_COMMON); }
  bool is_weak() const { return st_bind() == to_underlying(STB_WEAK); }
  bool is_undef_weak() const { return is_undef() && is_weak(); }
  constexpr SymbolType st_type() const noexcept { return SymbolType(st_info & 0x0F); }
  constexpr void set_type(SymbolType t) noexcept { st_info = (st_info & 0xF0) | (to_underlying(t) & 0xF); }
  constexpr SymbolBinding st_bind() const noexcept { return SymbolBinding((st_info >> 4) & 0x0F); }
  constexpr void set_bind(SymbolBinding b) noexcept { st_info = ((to_underlying(b) & 0xF) << 4) | (st_info & 0x0F); }
  constexpr SymbolVisibility st_visibility() const noexcept { return SymbolVisibility(st_other & 0x3); }
  constexpr void set_visibility(SymbolVisibility v) noexcept {
    st_other = (st_other & 0xFC) | (to_underlying(v) & 0x3);
  }
  U32<E> st_name;
  Word<B, E> st_value;
  Word<B, E> st_size;
  u8 st_info;  // See: SymbolBinding, SymbolType
  u8 st_other; // See: SymbolVisibility
  U16<E> st_shndx;
};
#pragma pack(pop)

using ElfSymbolLE32 = ElfSymbol<ElfBits::b32, ElfEndian::le>;
using ElfSymbolLE64 = ElfSymbol<ElfBits::b64, ElfEndian::le>;
using ElfSymbolBE32 = ElfSymbol<ElfBits::b32, ElfEndian::be>;
using ElfSymbolBE64 = ElfSymbol<ElfBits::b64, ElfEndian::be>;

} // namespace pepp::bts
