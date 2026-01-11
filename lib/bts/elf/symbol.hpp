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

template <typename E> struct ElfSymbol {
  using enum SectionIndices;
  using enum SymbolBinding;
  bool is_undef() const { return st_shndx == to_underlying(SHN_UNDEF); }
  bool is_abs() const { return st_shndx == to_underlying(SHN_ABS); }
  bool is_common() const { return st_shndx == to_underlying(SHN_COMMON); }
  bool is_weak() const { return st_bind == to_underlying(STB_WEAK); }
  bool is_undef_weak() const { return is_undef() && is_weak(); }
  u8 info() const { return (st_bind << 4) | (st_type & 0x0F); }

  U32<E> st_name;

#ifdef __LITTLE_ENDIAN__
  u8 st_type : 4; // See: SymbolType
  u8 st_bind : 4; // See: SymbolBinding
  union {
    u8 st_visibility : 2; // See: SymbolVisibility
    struct {
      u8 : 7;
      u8 arm64_variant_pcs : 1;
    };
    struct {
      u8 : 7;
      u8 riscv_variant_cc : 1;
    };
  };
#else
  u8 st_bind : 4; // See: SymbolBinding
  u8 st_type : 4; // See SymbolType
  union {
    struct {
      u8 : 6;
      u8 st_visibility : 2; // See: SymbolVisibility
    };
    u8 arm64_variant_pcs : 1;
    u8 riscv_variant_cc : 1;
  };
#endif

  U16<E> st_shndx;
  Word<E> st_value;
  Word<E> st_size;
};

} // namespace pepp::bts
