#pragma once
#include "./packed_elf.hpp"
#include "bts/elf/packed_fixup.hpp"
#include "packed_access_strings.hpp"
#include "packed_ops.hpp"
namespace pepp::bts {

template <ElfBits B, ElfEndian E, bool Const> class PackedSymbolAccessor {
public:
  using Elf = maybe_const_t<Const, PackedElf<B, E>>;
  using Shdr = maybe_const_t<Const, PackedElfShdr<B, E>>;
  using Data = maybe_const_t<Const, std::vector<u8>>;

  PackedSymbolAccessor(Elf &elf, u16 index);
  PackedSymbolAccessor(Shdr &shdr_symbol, Data &symtab, Shdr &shdr, Data &strtab) noexcept;

  u32 symbol_count() const noexcept;
  PackedElfSymbol<B, E> get_symbol(u32 index) const noexcept;
  PackedElfSymbol<B, E> *get_symbol_ptr(u32 index) const noexcept;
  bits::span<const char> get_symbol_name(u32 index) const noexcept;
  u32 find_symbol(std::string_view name) const noexcept;
  u32 find_symbol(Word<B, E> value) const noexcept;

  // Add a symbol to the table. Assumes you already set st_name!
  u32 add_symbol(PackedElfSymbol<B, E> &&symbol);
  // Helpers to assign name and section index.
  u32 add_symbol(PackedElfSymbol<B, E> &&symbol, std::string_view name);
  u32 add_symbol(PackedElfSymbol<B, E> &&symbol, std::string_view name, u16 section_index);
  void replace_symbol(u32 index, PackedElfSymbol<B, E> &&symbol) noexcept;
  void replace_value(u32 index, Word<B, E> value) noexcept;
  void arrange_local_symbols(std::function<void(Word<B, E> first, Word<B, E> second)> func = nullptr);

protected:
  void copy_to_symtab(PackedElfSymbol<B, E> &&symbol);
  void swap_symbols(u32 first, u32 second) noexcept;
  Shdr &shdr_symtab;
  Data &data_symtab;
  PackedStringAccessor<B, E, Const> strtab;
};
template <ElfBits B, ElfEndian E> using PackedSymbolReader = PackedSymbolAccessor<B, E, true>;
template <ElfBits B, ElfEndian E> using PackedSymbolWriter = PackedSymbolAccessor<B, E, false>;

// DANGER! The index will not be updated by arrange_local_symbols.
template <ElfBits B, ElfEndian E>
AbsoluteFixup fixup_symbol_value(PackedElf<B, E> &elf, u16 section, word<B> index, std::function<word<B>()> func) {
  return AbsoluteFixup{.update = [elf, section, index, func]() {
    PackedSymbolReader<B, E> t(elf, section);
    if (index >= t.symbol_count()) return;
    PackedElfSymbol<B, E> *sym = t.get_symbol_ptr(index);
    sym->st_value = func();
  }};
}

template <ElfBits B, ElfEndian E, bool Const> class PackedSymbolVersionAccessor {
public:
  using Elf = maybe_const_t<Const, PackedElf<B, E>>;
  using Shdr = maybe_const_t<Const, PackedElfShdr<B, E>>;
  using Data = maybe_const_t<Const, std::vector<u8>>;
  PackedSymbolVersionAccessor(Elf &elf, u16 index);
  u32 version_count() const noexcept;
  u16 get_version(u32 index) const noexcept;
  void set_version(u32 index, u16 version) noexcept;

private:
  Shdr &shdr;
  Data &data;
  PackedSymbolAccessor<B, E, Const> dynamic_symtab;
};
template <ElfBits B, ElfEndian E> using PackedSymbolVersionReader = PackedSymbolVersionAccessor<B, E, true>;
template <ElfBits B, ElfEndian E> using PackedSymbolVersionWriter = PackedSymbolVersionAccessor<B, E, false>;

template <ElfBits B, ElfEndian E, bool Const> class PackedSymbolNeedAccessor {
public:
  using Elf = maybe_const_t<Const, PackedElf<B, E>>;
  using Shdr = maybe_const_t<Const, PackedElfShdr<B, E>>;
  using Data = maybe_const_t<Const, std::vector<u8>>;
  PackedSymbolNeedAccessor(Elf &elf, u16 index) : shdr(elf.section_headers[index]), data(elf.section_data[index]) {
    // Walk the chain once to pre-compute needed count if we are a Reader.
    if constexpr (Const) {
      u64 offset = 0;
      while (offset + sizeof(PackedElfVerneed<E>) <= data.size()) {
        auto verneed = needed_from_offset(static_cast<u32>(offset));
        _offsets_for_needed.emplace_back(offset), _needed++;
        offset += verneed->vd_next;
      }
    }
  }
  u32 needed_count() const noexcept { return _needed; }
  u32 needed_offset_for_index(u32 index) const noexcept {
    if (_offsets_for_needed.size() > index) return _offsets_for_needed[index];

    u32 it = 0, offset = 0;
    while (offset + sizeof(PackedElfVerneed<E>) <= data.size() && it < index) {
      auto verneed = reinterpret_cast<const PackedElfVerneed<E> *>(data.data() + offset);
      it++;
      if (_offsets_for_needed.size() < index) _offsets_for_needed.emplace_back(offset);
      offset += verneed->vn_next;
    }
    return static_cast<u32>(offset);
  }
  PackedElfVerneed<E> needed_from_offset(u32 offset) const noexcept {
    if (offset + sizeof(PackedElfVerneed<E>) > data.size()) return {};
    return reinterpret_cast<const PackedElfVerneed<E> *>(data.data() + offset);
  }
  PackedElfVerneed<E> needed_from_index(u32 index) const noexcept {
    auto offset = needed_offset_for_index(index);
    return needed_from_offset(offset);
  }
  u32 aux_offset_for_index(u32 need_index, u32 aux_index) const noexcept {
    auto need = needed_from_index(need_index);
    u32 offset = static_cast<u32>(need.vd_aux);
    for (u32 it = 0; it < aux_index; ++it) {
      if (offset + sizeof(PackedElfVernaux<E>) > data.size()) return 0;
      offset += needed_aux_from_offset(offset).vda_next;
    }
    return offset;
  }
  PackedElfVernaux<E> needed_aux_from_offset(u32 offset) const noexcept {
    if (offset + sizeof(PackedElfVernaux<E>) > data.size()) return {};
    return reinterpret_cast<const PackedElfVernaux<E> *>(data.data() + offset);
  }
  PackedElfVernaux<E> needed_aux_from_index(u32 need_index, u32 aux_index) const noexcept {
    auto offset = aux_offset_for_index(need_index, aux_index);
    return needed_aux_from_offset(offset);
  }

  u32 add_verneed(PackedElfVerneed<E> &&needed) {
    auto data_pos = data.size();
    if (_needed > 0) {
      auto prev = needed_offset_for_index(_needed);
      // Update previous vd_next
      auto prev_needed = reinterpret_cast<PackedElfVerneed<E> *>(data.data() + prev);
      prev_needed->vn_next = static_cast<u32>(data_pos - prev);
    }
    data.resize(data.size() + sizeof(PackedElfVerneed<E>));
    std::memcpy(data.data() + data_pos, &needed, sizeof(PackedElfVerneed<E>));
    _offsets_for_needed.emplace_back(data_pos);
    _needed++;
    shdr.sh_size = data.size();
    shdr.sh_info = _needed;
    return data_pos;
  }
  u32 add_vernaux(u32 need_index, PackedElfVernaux<E> &&aux) {
    if (_needed == 0) return 0;
    auto need_offset = needed_offset_for_index(need_index);
    auto data_pos = data.size();
    auto need = reinterpret_cast<PackedElfVerneed<E> *>(data.data() + need_offset);
    auto aux_offset = need->vn_aux;
    if (aux_offset != 0) {
      // Walk to the end of the aux chain
      u32 last_aux_offset = aux_offset;
      PackedElfVernaux<E> *last_aux = nullptr;
      while (last_aux_offset + sizeof(PackedElfVernaux<E>) <= data.size()) {
        last_aux = reinterpret_cast<PackedElfVernaux<E> *>(data.data() + last_aux_offset);
        if (last_aux->vna_next == 0) break;
        last_aux_offset += last_aux->vna_next;
      }
      if (last_aux != nullptr) last_aux->vna_next = static_cast<u32>(data_pos - last_aux_offset);
    } else {
      // First aux for this needed entry
      auto offset = static_cast<u32>(data_pos - need_offset);
      need->vn_aux = offset;
    }
    data.resize(data.size() + sizeof(PackedElfVernaux<E>));
    std::memcpy(data.data() + data_pos, &aux, sizeof(PackedElfVernaux<E>));
    shdr.sh_size = data.size();
    return data_pos;
  }

private:
  Shdr &shdr;
  Data &data;
  u32 _needed = 0;
  mutable std::vector<u32> _offsets_for_needed;
};
template <ElfBits B, ElfEndian E> using PackedSymbolNeedReader = PackedSymbolNeedAccessor<B, E, true>;
template <ElfBits B, ElfEndian E> using PackedSymbolNeedWriter = PackedSymbolNeedAccessor<B, E, false>;

// Symbols

template <ElfBits B, ElfEndian E, bool Const>
PackedSymbolAccessor<B, E, Const>::PackedSymbolAccessor(PackedSymbolAccessor<B, E, Const>::Elf &elf, u16 index)
    : shdr_symtab(elf.section_headers[index]), data_symtab(elf.section_data[index]), strtab(elf, shdr_symtab.sh_link) {
  if (index > elf.section_headers.size()) throw std::runtime_error("PackedSymbolWriter: invalid section index");
}

template <ElfBits B, ElfEndian E, bool Const>
PackedSymbolAccessor<B, E, Const>::PackedSymbolAccessor(PackedSymbolAccessor<B, E, Const>::Shdr &shdr_symbol,
                                                        PackedSymbolAccessor<B, E, Const>::Data &symtab,
                                                        PackedSymbolAccessor<B, E, Const>::Shdr &shdr,
                                                        PackedSymbolAccessor<B, E, Const>::Data &strtab) noexcept
    : shdr_symtab(shdr_symbol), data_symtab(symtab), strtab(shdr, strtab) {}

template <ElfBits B, ElfEndian E, bool Const> u32 PackedSymbolAccessor<B, E, Const>::symbol_count() const noexcept {
  return pepp::bts::symbol_count<B, E>(shdr_symtab);
}

template <ElfBits B, ElfEndian E, bool Const>
PackedElfSymbol<B, E> PackedSymbolAccessor<B, E, Const>::get_symbol(u32 index) const noexcept {
  auto ptr = get_symbol_ptr(index);
  if (ptr == nullptr) return {};
  return *ptr;
}

template <ElfBits B, ElfEndian E, bool Const>
PackedElfSymbol<B, E> *PackedSymbolAccessor<B, E, Const>::get_symbol_ptr(u32 index) const noexcept {
  if (index >= symbol_count()) return nullptr;
  auto ret = (PackedElfSymbol<B, E> *)(data_symtab.data() + index * shdr_symtab.sh_entsize);
  return ret;
}

template <ElfBits B, ElfEndian E, bool Const>
bits::span<const char> PackedSymbolAccessor<B, E, Const>::get_symbol_name(u32 index) const noexcept {
  if (index >= symbol_count()) return {};
  return strtab.get_string_span(get_symbol(index).st_name);
}

template <ElfBits B, ElfEndian E, bool Const>
u32 PackedSymbolAccessor<B, E, Const>::find_symbol(std::string_view name) const noexcept {
  auto str_idx = strtab.find(name);
  if (str_idx == 0 && !name.empty()) return 0;
  for (u32 it = 0; it < symbol_count(); ++it)
    if (auto sym = get_symbol(it); sym.st_name == str_idx) return it;
  return 0;
}

template <ElfBits B, ElfEndian E, bool Const>
u32 PackedSymbolAccessor<B, E, Const>::find_symbol(Word<B, E> address) const noexcept {
  for (u32 it = 0; it < symbol_count(); ++it)
    if (auto sym = get_symbol(it); sym.st_value == address) return it;
  return 0;
}

template <ElfBits B, ElfEndian E, bool Const>
u32 PackedSymbolAccessor<B, E, Const>::add_symbol(PackedElfSymbol<B, E> &&symbol) {
  if (shdr_symtab.sh_size == 0) copy_to_symtab(create_null_symbol<B, E>());
  copy_to_symtab(std::move(symbol));
  return symbol_count() - 1;
}

template <ElfBits B, ElfEndian E, bool Const>
u32 PackedSymbolAccessor<B, E, Const>::add_symbol(PackedElfSymbol<B, E> &&symbol, std::string_view name) {
  auto idx = symbol.st_shndx;
  add_symbol(std::move(symbol), name, idx);
  return symbol_count() - 1;
}

template <ElfBits B, ElfEndian E, bool Const>
u32 PackedSymbolAccessor<B, E, Const>::add_symbol(PackedElfSymbol<B, E> &&symbol, std::string_view name,
                                                  u16 section_index) {
  if (shdr_symtab.sh_size == 0) copy_to_symtab(create_null_symbol<B, E>());
  auto name_idx = strtab.add_string(name);
  symbol.st_name = name_idx;
  symbol.st_shndx = section_index;
  copy_to_symtab(std::move(symbol));
  return symbol_count() - 1;
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedSymbolAccessor<B, E, Const>::replace_symbol(u32 index, PackedElfSymbol<B, E> &&symbol) noexcept {
  if (index >= symbol_count()) return;
  *get_symbol_ptr(index) = symbol;
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedSymbolAccessor<B, E, Const>::replace_value(u32 index, Word<B, E> value) noexcept {
  if (index >= symbol_count()) return;
  get_symbol_ptr(index)->st_value = value;
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedSymbolAccessor<B, E, Const>::arrange_local_symbols(std::function<void(Word<B, E>, Word<B, E>)> func) {
  u32 first_not_local = 1; // Skip the first entry. It is always NOTYPE
  word<B> current = 0, count = symbol_count();

  while (true) {
    PackedElfSymbol<B, E> *p1 = nullptr;
    PackedElfSymbol<B, E> *p2 = nullptr;

    while (first_not_local < count) {
      p1 = get_symbol_ptr(first_not_local);
      if (p1->st_bind() != SymbolBinding::STB_LOCAL) break;
      ++first_not_local;
    }

    current = first_not_local + 1;
    while (current < count) {
      p2 = get_symbol_ptr(current);
      if (p2->st_bind() == SymbolBinding::STB_LOCAL) break;
      ++current;
    }

    if (first_not_local < count && current < count) {
      if (func) func(first_not_local, current);
      swap_symbols(first_not_local, current);
    } else {
      // Update 'info' field of the section
      shdr_symtab.sh_info = first_not_local;
      break;
    }
  }
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedSymbolAccessor<B, E, Const>::copy_to_symtab(PackedElfSymbol<B, E> &&symbol) {
  if (shdr_symtab.sh_entsize == 0) shdr_symtab.sh_entsize = sizeof(PackedElfSymbol<B, E>);
  const auto ate = data_symtab.size();
  data_symtab.resize(data_symtab.size() + sizeof(PackedElfSymbol<B, E>));
  std::memcpy(data_symtab.data() + ate, reinterpret_cast<const u8 *>(&symbol), sizeof(PackedElfSymbol<B, E>));
  shdr_symtab.sh_size = data_symtab.size();
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedSymbolAccessor<B, E, Const>::swap_symbols(u32 first, u32 second) noexcept {
  const auto cnt = symbol_count();
  // Don't allow swapping the null symbol
  if (first >= cnt || second >= cnt || first == 0 || second == 0) return;
  auto ptr1 = get_symbol_ptr(first), ptr2 = get_symbol_ptr(second);
  std::swap(*ptr1, *ptr2);
}

template <ElfBits B, ElfEndian E, bool Const>
PackedSymbolVersionAccessor<B, E, Const>::PackedSymbolVersionAccessor(Elf &elf, u16 index)
    : shdr(elf.section_headers[index]), data(elf.section_data[index]), dynamic_symtab(elf, shdr.sh_link) {
  // The size of this section should be the size of the dynamic symbol table * sh_entsize.
  if constexpr (!Const) {
    shdr.sh_entsize = sizeof(u16);
    data.resize(dynamic_symtab.symbol_count() * shdr.sh_entsize, 0);
    shdr.sh_size = data.size();
  }
}

template <ElfBits B, ElfEndian E, bool Const>
u32 PackedSymbolVersionAccessor<B, E, Const>::version_count() const noexcept {
  return dynamic_symtab.symbol_count();
}

template <ElfBits B, ElfEndian E, bool Const>
u16 PackedSymbolVersionAccessor<B, E, Const>::get_version(u32 index) const noexcept {
  if (index >= version_count() || 1 + index * shdr.sh_entsize >= data.size()) return 0;
  return (U16<E> *)data.data() + index;
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedSymbolVersionAccessor<B, E, Const>::set_version(u32 index, u16 version) noexcept {
  if (index >= version_count() || index * shdr.sh_entsize >= data.size()) return;
  *((U16<E> *)data.data() + index) = version;
}

} // namespace pepp::bts
