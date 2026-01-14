#pragma once
#include "./packed_elf.hpp"
#include "bts/bitmanip/log2.hpp"
#include "packed_access_symbol.hpp"
namespace pepp::bts {
// Handles both REL and RELA.
// You just need to know which one you're dealing with.
template <ElfBits B, ElfEndian E, bool Const> class PackedRelocationAccessor {
public:
  using Elf = maybe_const_t<Const, PackedElf<B, E>>;
  using Shdr = maybe_const_t<Const, PackedElfShdr<B, E>>;
  using Data = maybe_const_t<Const, std::vector<u8>>;

  PackedRelocationAccessor(Elf &elf, u16 index);
  PackedRelocationAccessor(Shdr &shdr_reloc, Data &reloc, const PackedElfShdr<B, E> &shdr_symbol,
                           const std::vector<u8> &symtab, const PackedElfShdr<B, E> &shdr_strtab,
                           std::vector<u8> &strtab) noexcept;
  bool is_rela() const noexcept;
  u32 relocation_count() const noexcept;
  PackedElfRel<B, E> get_rel(u32 index) const noexcept;
  PackedElfRelA<B, E> get_rela(u32 index) const noexcept;

  void add_rel(PackedElfRel<B, E> &&rel);
  void add_rel(word<B> offset, u32 type, u32 symbol);
  void add_rel(word<B> offset, u32 type, std::string_view name);
  void replace_rel(u32 index, word<B> offset, u32 type, u32 symbol);

  void add_rela(PackedElfRelA<B, E> &&rel);
  void add_rela(word<B> offset, u32 type, u32 symbol, sword<B> addend);
  void add_rela(word<B> offset, u32 type, std::string_view name, sword<B> addend);
  void replace_rela(u32 index, word<B> offset, u32 type, u32 symbol, sword<B> addend);

  void swap_symbols(u32 first, u32 second) noexcept;

private:
  Shdr &shdr;
  Data &reloc;
  PackedSymbolReader<B, E> symtab;
};
template <ElfBits B, ElfEndian E> using PackedRelocationReader = PackedRelocationAccessor<B, E, true>;
template <ElfBits B, ElfEndian E> using PackedRelocationWriter = PackedRelocationAccessor<B, E, false>;

// Relocations
template <ElfBits B, ElfEndian E, bool Const>
PackedRelocationAccessor<B, E, Const>::PackedRelocationAccessor(PackedRelocationAccessor<B, E, Const>::Elf &elf,
                                                                u16 index)
    : shdr(elf.section_headers[index]), reloc(elf.section_data[index]), symtab(elf, shdr.sh_link) {
  if (index > elf.section_headers.size()) throw std::runtime_error("PackedRelocationWriter: invalid section index");
}

template <ElfBits B, ElfEndian E, bool Const>
PackedRelocationAccessor<B, E, Const>::PackedRelocationAccessor(PackedRelocationAccessor<B, E, Const>::Shdr &shdr_reloc,
                                                                PackedRelocationAccessor<B, E, Const>::Data &reloc,
                                                                const PackedElfShdr<B, E> &shdr_symbol,
                                                                const std::vector<u8> &symtab,
                                                                const PackedElfShdr<B, E> &shdr_strtab,
                                                                std::vector<u8> &strtab) noexcept
    : shdr(shdr_reloc), reloc(reloc), symtab(symtab, shdr_symbol, shdr_strtab, strtab) {}

template <ElfBits B, ElfEndian E, bool Const> bool PackedRelocationAccessor<B, E, Const>::is_rela() const noexcept {
  return (shdr.sh_type == to_underlying(SectionTypes::SHT_RELA));
}

template <ElfBits B, ElfEndian E, bool Const>
u32 PackedRelocationAccessor<B, E, Const>::relocation_count() const noexcept {
  if (shdr.sh_entsize == 0 || shdr.sh_size == 0) return 0;
  return shdr.sh_size / shdr.sh_entsize;
}

template <ElfBits B, ElfEndian E, bool Const>
PackedElfRel<B, E> PackedRelocationAccessor<B, E, Const>::get_rel(u32 index) const noexcept {
  if (index >= relocation_count()) return PackedElfRel<B, E>{};
  return *(PackedElfRel<B, E> *)(reloc.data() + index * shdr.sh_entsize);
}

template <ElfBits B, ElfEndian E, bool Const>
PackedElfRelA<B, E> PackedRelocationAccessor<B, E, Const>::get_rela(u32 index) const noexcept {
  if (index >= relocation_count()) return PackedElfRelA<B, E>{};
  return *(PackedElfRelA<B, E> *)(reloc.data() + index * shdr.sh_entsize);
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedRelocationAccessor<B, E, Const>::add_rel(PackedElfRel<B, E> &&rel) {
  if (shdr.sh_entsize == 0) shdr.sh_entsize = sizeof(PackedElfRel<B, E>);
  const u8 *ptr = reinterpret_cast<const u8 *>(&rel);
  auto ate = reloc.size();
  reloc.resize(ate + sizeof(PackedElfRel<B, E>), 0);
  std::memcpy(reloc.data() + ate, &rel, sizeof(PackedElfRel<B, E>));
  shdr.sh_size = reloc.size();
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedRelocationAccessor<B, E, Const>::add_rel(word<B> offset, u32 type, u32 symbol) {
  PackedElfRel<B, E> rel;
  rel.r_offset = offset;
  if constexpr (B == ElfBits::b32) rel.r_info = pepp::bts::r_info<E>((u8)type, U24<E>{symbol});
  else rel.r_info = pepp::bts::r_info<E>(U32<E>{type}, U32<E>{symbol});
  add_rel(std::move(rel));
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedRelocationAccessor<B, E, Const>::add_rel(word<B> offset, u32 type, std::string_view name) {
  auto symbol = symtab.find_symbol(name);
  if (symbol == 0) throw std::runtime_error("PackedRelocationWriter: symbol not found: " + std::string(name));
  PackedElfRel<B, E> rel;
  rel.r_offset = offset;
  if constexpr (B == ElfBits::b32) rel.r_info = pepp::bts::r_info<E>((u8)type, U24<E>{symbol});
  else rel.r_info = pepp::bts::r_info<E>(U32<E>{type}, U32<E>{symbol});

  add_rel(std::move(rel));
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedRelocationAccessor<B, E, Const>::replace_rel(u32 index, word<B> offset, u32 type, u32 symbol) {
  if (index >= relocation_count()) throw std::runtime_error("PackedRelocationWriter: invalid relocation index");
  PackedElfRel<B, E> *rel = reinterpret_cast<PackedElfRel<B, E> *>(reloc.data() + index * shdr.sh_entsize);
  rel->r_offset = offset;
  if constexpr (B == ElfBits::b32) rel->r_info = pepp::bts::r_info<E>((u8)type, U24<E>{symbol});
  else rel->r_info = pepp::bts::r_info<E>(U32<E>{type}, U32<E>{symbol});
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedRelocationAccessor<B, E, Const>::add_rela(PackedElfRelA<B, E> &&rel) {
  if (shdr.sh_entsize == 0) shdr.sh_entsize = sizeof(PackedElfRelA<B, E>);
  auto ate = reloc.size();
  reloc.resize(ate + sizeof(PackedElfRelA<B, E>), 0);
  std::memcpy(reloc.data() + ate, &rel, sizeof(PackedElfRelA<B, E>));
  shdr.sh_size = reloc.size();
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedRelocationAccessor<B, E, Const>::add_rela(word<B> offset, u32 type, u32 symbol, sword<B> addend) {
  PackedElfRelA<B, E> rela;
  rela.r_offset = offset;
  if constexpr (B == ElfBits::b32) rela.r_info = pepp::bts::r_info<E>((u8)type, U24<E>{symbol});
  else rela.r_info = pepp::bts::r_info<E>(U32<E>{type}, U32<E>{symbol});
  rela.r_addend = addend;
  add_rela(std::move(rela));
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedRelocationAccessor<B, E, Const>::add_rela(word<B> offset, u32 type, std::string_view name, sword<B> addend) {
  auto symbol = symtab.find_symbol_index(name);
  if (symbol == 0) throw std::runtime_error("PackedRelocationWriter: symbol not found: " + std::string(name));
  PackedElfRelA<B, E> rela;
  rela.r_offset = offset;
  if constexpr (B == ElfBits::b32) rela.r_info = pepp::bts::r_info<E>((u8)type, U24<E>{symbol});
  else rela.r_info = pepp::bts::r_info<E>(U32<E>{type}, U32<E>{symbol});
  rela.r_addend = addend;
  add_rela(std::move(rela));
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedRelocationAccessor<B, E, Const>::replace_rela(u32 index, word<B> offset, u32 type, u32 symbol,
                                                         sword<B> addend) {
  if (index >= relocation_count()) throw std::runtime_error("PackedRelocationWriter: invalid relocation index");
  PackedElfRelA<B, E> *rela = reinterpret_cast<PackedElfRelA<B, E> *>(reloc.data() + index * shdr.sh_entsize);
  rela->r_offset = offset;
  if constexpr (B == ElfBits::b32) rela->r_info = pepp::bts::r_info<E>((u8)type, U24<E>{symbol});
  else rela->r_info = pepp::bts::r_info<E>(U32<E>{type}, U32<E>{symbol});
  rela->r_addend = addend;
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedRelocationAccessor<B, E, Const>::swap_symbols(u32 first, u32 second) noexcept {
  // Helpers to do the 32- vs 64-bit packing for REL/RELA type|symbol
  static const auto update_rel = [](PackedElfRel<B, E> *rela, u32 new_sym) {
    if constexpr (B == ElfBits::b32) {
      auto type = pepp::bts::r_type<E>(U32<E>{rela->r_info});
      rela->r_info = pepp::bts::r_info<E>((u8)type, U24<E>{new_sym});
    } else {
      auto type = pepp::bts::r_type<E>(U64<E>{rela->r_info});
      rela->r_info = pepp::bts::r_info<E>(U32<E>{type}, U32<E>{new_sym});
    }
  };
  static const auto update_rela = [](PackedElfRelA<B, E> *rela, u32 new_sym) {
    if constexpr (B == ElfBits::b32) {
      auto type = pepp::bts::r_type<E>(U32<E>{rela->r_info});
      rela->r_info = pepp::bts::r_info<E>((u8)type, U24<E>{new_sym});
    } else {
      auto type = pepp::bts::r_type<E>(U64<E>{rela->r_info});
      rela->r_info = pepp::bts::r_info<E>(U32<E>{type}, U32<E>{new_sym});
    }
  };

  for (int it = 0; it < relocation_count(); it++) {
    if (is_rela()) {
      PackedElfRelA<B, E> *rela = reinterpret_cast<PackedElfRelA<B, E> *>(reloc.data() + it * shdr.sh_entsize);
      u32 sym = B == ElfBits::b64 ? r_sym<E>(U64<E>{rela->r_info}) : r_sym<E>(U32<E>{rela->r_info});
      if (sym == first) update_rela(rela, second);
      else if (sym == second) update_rela(rela, first);
    } else {
      PackedElfRel<B, E> *rel = reinterpret_cast<PackedElfRel<B, E> *>(reloc.data() + it * shdr.sh_entsize);
      u32 sym = B == ElfBits::b64 ? r_sym<E>(U64<E>{rel->r_info}) : r_sym<E>(U32<E>{rel->r_info});
      if (sym == first) update_rel(rel, second);
      else if (sym == second) update_rel(rel, first);
    }
  }
}
} // namespace pepp::bts
