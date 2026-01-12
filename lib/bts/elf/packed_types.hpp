#pragma once
#include <span>
#include "../bitmanip/integers.h"
#include "bts/elf/enums.hpp"

/*
 * The datatypes within this file are standard-layout structs. If you mmap an ELF file into memory, you should be able
 * to cast (modulo alignment constrains, use memcpy) directly to these types to read the data. These classes handle
 * converting between host endianess and ELF endianness via the use of bitmanip integer types.
 * Where formats differ in 32 vs 64 bit ELF files, template specialization is used.
 *
 * The library will not help you maintain ELF invariants (e.g.,, section offsets/sizes, header counts, etc).
 * If you want to create valid ELF files, you will need to manage those details yourself. The alternative
 * "Managed" API can help create valid ELF files more easily, at the expense of additional processing and memory.
 */
namespace pepp::bts {

// Not real enumerated values as specified in the ELF spec, instead used to constrain template parameters.
enum class ElfBits : u8 { b32, b64 };
enum class ElfEndian : u8 { le, be };

// Data types which are conditionally endianness-reversed.
template <ElfEndian E> using I16 = std::conditional_t<E == ElfEndian::le, il16, ib16>;
template <ElfEndian E> using I32 = std::conditional_t<E == ElfEndian::le, il32, ib32>;
template <ElfEndian E> using I64 = std::conditional_t<E == ElfEndian::le, il64, ib64>;
template <ElfEndian E> using U16 = std::conditional_t<E == ElfEndian::le, ul16, ub16>;
template <ElfEndian E> using U24 = std::conditional_t<E == ElfEndian::le, ul24, ub24>;
template <ElfEndian E> using U32 = std::conditional_t<E == ElfEndian::le, ul32, ub32>;
template <ElfEndian E> using U64 = std::conditional_t<E == ElfEndian::le, ul64, ub64>;

// Per: ELF TIS Figure 1-2, 1-5, 1-6
template <ElfBits B, ElfEndian E> using Word = std::conditional_t<B == ElfBits::b64, U64<E>, U32<E>>;
template <ElfBits B, ElfEndian E> using SWord = std::conditional_t<B == ElfBits::b64, I64<E>, I32<E>>;
template <ElfBits B> using word = std::conditional_t<B == ElfBits::b64, u64, u32>;

#pragma pack(push, 1)
// Per: ELF TIS Figure 1-3
template <ElfBits B, ElfEndian E> struct PackedElfEhdr {
  // Create mostly-0-initialized ELF header
  PackedElfEhdr() noexcept;
  PackedElfEhdr(ElfFileType type, ElfMachineType machine, ElfABI abi) noexcept;

  u8 e_ident[16];
  U16<E> e_type;    // See: FileType
  U16<E> e_machine; // See: MachineType
  U32<E> e_version; // See: ElfVersion
  Word<B, E> e_entry;
  Word<B, E> e_phoff;
  Word<B, E> e_shoff;
  U32<E> e_flags;
  U16<E> e_ehsize;
  U16<E> e_phentsize;
  U16<E> e_phnum;
  U16<E> e_shentsize;
  U16<E> e_shnum;
  U16<E> e_shstrndx;
};
using PackedElfEhdrLE32 = PackedElfEhdr<ElfBits::b32, ElfEndian::le>;
using PackedElfEhdrLE64 = PackedElfEhdr<ElfBits::b64, ElfEndian::le>;
using PackedElfEhdrBE32 = PackedElfEhdr<ElfBits::b32, ElfEndian::be>;
using PackedElfEhdrBE64 = PackedElfEhdr<ElfBits::b64, ElfEndian::be>;

// Per: ELF TIS Figure 1-8
template <ElfBits B, ElfEndian E> struct PackedElfShdr {
  PackedElfShdr() noexcept;

  U32<E> sh_name;
  U32<E> sh_type;      // See: SectionTypes
  Word<B, E> sh_flags; // See: SectionFlags
  Word<B, E> sh_addr;
  Word<B, E> sh_offset;
  Word<B, E> sh_size;
  U32<E> sh_link;
  U32<E> sh_info;
  Word<B, E> sh_addralign;
  Word<B, E> sh_entsize;
};
using PackedElfShdrLE32 = PackedElfShdr<ElfBits::b32, ElfEndian::le>;
using PackedElfShdrLE64 = PackedElfShdr<ElfBits::b64, ElfEndian::le>;
using PackedElfShdrBE32 = PackedElfShdr<ElfBits::b32, ElfEndian::be>;
using PackedElfShdrBE64 = PackedElfShdr<ElfBits::b64, ElfEndian::be>;

// Per: ELF TIS Figure 1-15
template <ElfBits B, ElfEndian E> struct PackedElfSymbol;
// Symbol layout changes between 32/64 bits for alignment reasons.
template <ElfEndian E> struct PackedElfSymbol<ElfBits::b64, E> {
  PackedElfSymbol() noexcept;

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
  u8 st_info;  // See: SymbolBinding, SymbolType
  u8 st_other; // See: SymbolVisibility
  U16<E> st_shndx;
  U64<E> st_value;
  U64<E> st_size;
};
template <ElfEndian E> struct PackedElfSymbol<ElfBits::b32, E> {
  PackedElfSymbol() noexcept;

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
  U32<E> st_value;
  U32<E> st_size;
  u8 st_info;  // See: SymbolBinding, SymbolType
  u8 st_other; // See: SymbolVisibility
  U16<E> st_shndx;
};
using PackedElfSymbolLE32 = PackedElfSymbol<ElfBits::b32, ElfEndian::le>;
using PackedElfSymbolLE64 = PackedElfSymbol<ElfBits::b64, ElfEndian::le>;
using PackedElfSymbolBE32 = PackedElfSymbol<ElfBits::b32, ElfEndian::be>;
using PackedElfSymbolBE64 = PackedElfSymbol<ElfBits::b64, ElfEndian::be>;

// Per: sidebar below ELF TIS Figure 1-19
template <ElfEndian E> u8 r_type(U32<E> r_info) noexcept { return static_cast<u8>(r_info & 0xFF); }
template <ElfEndian E> U24<E> r_sym(U32<E> r_info) noexcept { return U24<E>((r_info >> 8) & 0xFFFFFF); }
template <ElfEndian E> U32<E> r_info(u8 r_type, U24<E> r_sym) noexcept {
  return U32<E>((static_cast<U32<E>>(r_sym) << 8) | static_cast<U32<E>>(r_type));
}
template <ElfEndian E> U32<E> r_type(U64<E> r_info) noexcept { return U32<E>(r_info & 0xFFFFFFFF); }
template <ElfEndian E> U32<E> r_sym(U64<E> r_info) noexcept { return U32<E>((r_info >> 32) & 0xFFFFFFFF); }
template <ElfEndian E> U64<E> r_info(U32<E> r_type, U32<E> r_sym) noexcept {
  return U64<E>((static_cast<U64<E>>(r_sym) << 32) | static_cast<U64<E>>(r_type));
}

// Per: ELF TIS Figure 1-19
template <ElfBits B, ElfEndian E> struct PackedElfRel;
template <ElfEndian E> struct PackedElfRel<ElfBits::b32, E> {
  PackedElfRel() = default;
  PackedElfRel(u64 offset, u32 type, u32 sym) : r_offset(offset), r_info(pepp::bts::r_info<E>(type, sym)) {}
  constexpr u8 r_type() const noexcept { return pepp::bts::r_type(r_info); }
  constexpr U24<E> r_sym() const noexcept { return pepp::bts::r_sym(r_info); }
  constexpr void set_r_type(u8 type) noexcept { r_info = pepp::bts::r_info(type, r_sym()); }
  constexpr void set_r_sym(U24<E> sym) noexcept { r_info = pepp::bts::r_info(r_type(), sym); }

  Word<ElfBits::b32, E> r_offset;
  U32<E> r_info;
};
template <ElfEndian E> struct PackedElfRel<ElfBits::b64, E> {
  PackedElfRel() = default;
  PackedElfRel(u64 offset, u32 type, u32 sym) : r_offset(offset), r_info(pepp::bts::r_info<E>(type, sym)) {}
  constexpr U32<E> r_type() const noexcept { return pepp::bts::r_type(r_info); }
  constexpr U32<E> r_sym() const noexcept { return pepp::bts::r_sym(r_info); }
  constexpr void set_r_type(U32<E> type) noexcept { r_info = pepp::bts::r_info(type, r_sym()); }
  constexpr void set_r_sym(U32<E> sym) noexcept { r_info = pepp::bts::r_info(r_type(), sym); }

  Word<ElfBits::b64, E> r_offset;
  U64<E> r_info;
};

// Per: ELF TIS Figure 1-19
template <ElfBits B, ElfEndian E> struct PackedElfRelA;
template <ElfEndian E> struct PackedElfRelA<ElfBits::b32, E> {
  PackedElfRelA() = default;
  PackedElfRelA(u64 offset, u32 type, u32 sym, i64 addend)
      : r_offset(offset), r_info(pepp::bts::r_info<E>(type, sym)), r_addend(addend) {}
  constexpr u8 r_type() const noexcept { return pepp::bts::r_type(r_info); }
  constexpr U24<E> r_sym() const noexcept { return pepp::bts::r_sym(r_info); }
  constexpr void set_r_type(u8 type) noexcept { r_info = pepp::bts::r_info(type, r_sym()); }
  constexpr void set_r_sym(U24<E> sym) noexcept { r_info = pepp::bts::r_info(r_type(), sym); }

  Word<ElfBits::b32, E> r_offset;
  U32<E> r_info;
  SWord<ElfBits::b32, E> r_addend;
};
template <ElfEndian E> struct PackedElfRelA<ElfBits::b64, E> {
  PackedElfRelA() = default;
  PackedElfRelA(u64 offset, u32 type, u32 sym, i64 addend)
      : r_offset(offset), r_info(pepp::bts::r_info<E>(type, sym)), r_addend(addend) {}
  constexpr U32<E> r_type() const noexcept { return pepp::bts::r_type(r_info); }
  constexpr U32<E> r_sym() const noexcept { return pepp::bts::r_sym(r_info); }
  constexpr void set_r_type(U32<E> type) noexcept { r_info = pepp::bts::r_info(type, r_sym()); }
  constexpr void set_r_sym(U32<E> sym) noexcept { r_info = pepp::bts::r_info(r_type(), sym); }

  Word<ElfBits::b64, E> r_offset;
  U64<E> r_info;
  SWord<ElfBits::b64, E> r_addend;
};

// Per: ELF TIS Figure 2-1
template <ElfBits B, ElfEndian E> struct PackedElfPhdr;
// Phdr layout changes between 32/64 bits for alignment reasons.
template <ElfEndian E> struct PackedElfPhdr<ElfBits::b32, E> {
  PackedElfPhdr() noexcept;

  U32<E> p_type; // See: SegmentType
  U32<E> p_offset;
  U32<E> p_vaddr;
  U32<E> p_paddr;
  U32<E> p_filesz;
  U32<E> p_memsz;
  U32<E> p_flags; // See: SegmentFlags
  U32<E> p_align;
};
template <ElfEndian E> struct PackedElfPhdr<ElfBits::b64, E> {
  PackedElfPhdr() noexcept;

  U32<E> p_type; // See: SegmentType
  U32<E> p_flags;
  U64<E> p_offset;
  U64<E> p_vaddr;
  U64<E> p_paddr;
  U64<E> p_filesz;
  U64<E> p_memsz; // See: SegmentFlags
  U64<E> p_align;
};
using PackedElfPhdrLE32 = PackedElfPhdr<ElfBits::b32, ElfEndian::le>;
using PackedElfPhdrLE64 = PackedElfPhdr<ElfBits::b64, ElfEndian::le>;
using PackedElfPhdrBE32 = PackedElfPhdr<ElfBits::b32, ElfEndian::be>;
using PackedElfPhdrBE64 = PackedElfPhdr<ElfBits::b64, ElfEndian::be>;

#pragma pack(pop)

// Per: ELF TIS Figure 1-10
template <ElfBits B, ElfEndian E> PackedElfShdr<B, E> create_null_header() {
  PackedElfShdr<B, E> shdr;
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

template <ElfBits B, ElfEndian E> PackedElfShdr<B, E> create_shstrtab_header(u32 name) {
  PackedElfShdr<B, E> shdr;
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

// Per: ELF TIS Figure 1-18
template <ElfBits B, ElfEndian E> PackedElfSymbol<B, E> create_null_symbol() {
  PackedElfSymbol<B, E> sym;
  sym.st_name = 0;
  sym.st_value = 0;
  sym.st_size = 0;
  sym.st_info = 0;
  sym.st_other = 0;
  sym.st_shndx = to_underlying(SectionIndices::SHN_UNDEF);
  return sym;
}

/*
 * Implementations of member methods
 */

namespace detail {
// Per: ELF TIS Figure 1-4
constexpr void fill_e_ident(std::span<u8, 16> e_ident, ElfBits B, ElfEndian E, ElfABI abi, u8 abi_version) noexcept {
  e_ident[to_underlying(ElfIdentifierIndices::EI_MAG0)] = to_underlying(ElfMagic::ELFMAG0);
  e_ident[to_underlying(ElfIdentifierIndices::EI_MAG1)] = to_underlying(ElfMagic::ELFMAG1);
  e_ident[to_underlying(ElfIdentifierIndices::EI_MAG2)] = to_underlying(ElfMagic::ELFMAG2);
  e_ident[to_underlying(ElfIdentifierIndices::EI_MAG3)] = to_underlying(ElfMagic::ELFMAG3);
  e_ident[to_underlying(ElfIdentifierIndices::EI_CLASS)] =
      to_underlying(B == ElfBits::b64 ? ElfClass::ELFCLASS64 : ElfClass::ELFCLASS32);
  e_ident[to_underlying(ElfIdentifierIndices::EI_DATA)] =
      to_underlying(E == ElfEndian::le ? ElfEncoding::ELFDATA2LSB : ElfEncoding::ELFDATA2MSB);
  e_ident[to_underlying(ElfIdentifierIndices::EI_VERSION)] = to_underlying(ElfVersion::EV_CURRENT);
  e_ident[to_underlying(ElfIdentifierIndices::EI_OSABI)] = to_underlying(abi);
  e_ident[to_underlying(ElfIdentifierIndices::EI_ABIVERSION)] = abi_version;
  auto padding = e_ident.subspan(to_underlying(ElfIdentifierIndices::EI_PAD));
  memset(padding.data(), 0, padding.size());
}
} // namespace detail

template <ElfBits B, ElfEndian E> inline PackedElfEhdr<B, E>::PackedElfEhdr() noexcept {
  static_assert(std::is_standard_layout_v<PackedElfEhdr>);
  detail::fill_e_ident(e_ident, B, E, (ElfABI)0, 0);
  e_type = to_underlying(ElfFileType::ET_NONE);
  e_machine = to_underlying(ElfMachineType::EM_NONE);
  e_version = to_underlying(ElfVersion::EV_CURRENT);
  e_entry = 0;
  e_phoff = 0;
  e_shoff = 0;
  e_flags = 0;
  e_ehsize = sizeof(PackedElfEhdr<B, E>);
  e_phentsize = 0;
  e_phnum = 0;
  e_shentsize = 0;
  e_shnum = 0;
  e_shstrndx = 0;
}

template <ElfBits B, ElfEndian E>
inline PackedElfEhdr<B, E>::PackedElfEhdr(ElfFileType type, ElfMachineType machine, ElfABI abi) noexcept {
  static_assert(std::is_standard_layout_v<PackedElfEhdr>);
  detail::fill_e_ident(e_ident, B, E, abi, 0);
  e_type = to_underlying(type);
  e_machine = to_underlying(machine);
  e_version = to_underlying(ElfVersion::EV_CURRENT);
  e_entry = 0;
  e_phoff = 0;
  e_shoff = 0;
  e_flags = 0;
  e_ehsize = sizeof(PackedElfEhdr<B, E>);
  e_phentsize = 0;
  e_phnum = 0;
  e_shentsize = 0;
  e_shnum = 0;
  e_shstrndx = 0;
}

template <ElfBits B, ElfEndian E>
inline PackedElfShdr<B, E>::PackedElfShdr() noexcept
    : sh_name(0), sh_type(0), sh_flags(0), sh_addr(0), sh_offset(0), sh_size(0), sh_link(0), sh_info(0),
      sh_addralign(0), sh_entsize(0) {
  static_assert(std::is_standard_layout_v<PackedElfShdr>);
}

template <ElfEndian E>
inline PackedElfSymbol<ElfBits::b64, E>::PackedElfSymbol() noexcept
    : st_name(0), st_info(0), st_other(0), st_shndx(0), st_value(0), st_size(0) {
  static_assert(std::is_standard_layout_v<PackedElfSymbol>);
}

template <ElfEndian E>
inline PackedElfSymbol<ElfBits::b32, E>::PackedElfSymbol() noexcept
    : st_name(0), st_value(0), st_size(0), st_info(0), st_other(0), st_shndx(0) {
  static_assert(std::is_standard_layout_v<PackedElfSymbol>);
}

template <ElfEndian E>
inline PackedElfPhdr<ElfBits::b32, E>::PackedElfPhdr() noexcept
    : p_type(0), p_offset(0), p_vaddr(0), p_paddr(0), p_filesz(0), p_memsz(0), p_flags(0), p_align(0) {
  static_assert(std::is_standard_layout_v<PackedElfPhdr>);
}

template <ElfEndian E>
inline PackedElfPhdr<ElfBits::b64, E>::PackedElfPhdr() noexcept
    : p_type(0), p_flags(0), p_offset(0), p_vaddr(0), p_paddr(0), p_filesz(0), p_memsz(0), p_align(0) {
  static_assert(std::is_standard_layout_v<PackedElfPhdr>);
}

} // namespace pepp::bts
