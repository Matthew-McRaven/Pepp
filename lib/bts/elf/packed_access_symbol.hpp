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
#include "bts/elf/packed_access_strings.hpp"
#include "bts/elf/packed_elf.hpp"
#include "bts/elf/packed_fixup.hpp"
#include "bts/elf/packed_ops.hpp"
namespace pepp::bts {

template <ElfBits B, ElfEndian E, bool Const> class PackedSymbolAccessor {
public:
  using Elf = maybe_const_t<Const, PackedElf<B, E>>;
  using Shdr = maybe_const_t<Const, PackedElfShdr<B, E>>;
  using Data = maybe_const_t<Const, std::shared_ptr<AStorage>>;
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

// Accessor for .gnu.version
template <ElfBits B, ElfEndian E, bool Const> class PackedSymbolVersionAccessor {
public:
  using Elf = maybe_const_t<Const, PackedElf<B, E>>;
  using Shdr = maybe_const_t<Const, PackedElfShdr<B, E>>;
  using Data = maybe_const_t<Const, std::shared_ptr<AStorage>>;
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

// Base accessor for both .gnu.version_r and .gnu.version_d
template <ElfBits B, ElfEndian E, bool Const, typename Ver, typename VerAux> class PackedVersionChainAccessor {
public:
  using Elf = maybe_const_t<Const, PackedElf<B, E>>;
  using Shdr = maybe_const_t<Const, PackedElfShdr<B, E>>;
  using Data = maybe_const_t<Const, std::shared_ptr<AStorage>>;

  PackedVersionChainAccessor(Elf &elf, u16 index) : shdr(elf.section_headers[index]), data(elf.section_data[index]) {
    // Walk the chain once to pre-compute needed count if we are a Reader.
    if constexpr (Const) {
      u64 offset = 0;
      while (offset + sizeof(Ver) <= data->size()) {
        auto ver = ver_from_offset(static_cast<u32>(offset));
        _offsets_for_ver.emplace_back(offset), _vers++;
        offset += ver->next();
      }
    }
  }

  u32 version_count() const noexcept { return _vers; }

  u32 ver_offset_for_index(u32 index) const noexcept {
    if (_offsets_for_ver.size() > index) return _offsets_for_ver[index];

    u32 it = 0, offset = 0;
    while (offset + sizeof(Ver) <= data->size() && it < index) {
      auto ver = data->template get_at<const Ver>(offset);
      it++;
      if (_offsets_for_ver.size() < index) _offsets_for_ver.emplace_back(offset);
      offset += ver->next();
    }
    return static_cast<u32>(offset);
  }

  Ver *ver_from_offset(u32 offset) const noexcept {
    if (offset + sizeof(Ver) > data.size()) return {};
    return data->template get_at<const Ver>(offset);
  }
  Ver *ver_from_index(u32 index) const noexcept {
    auto offset = ver_offset_for_index(index);
    return ver_from_offset(offset);
  }
  u32 aux_offset_for_index(u32 ver_index, u32 aux_index) const noexcept {
    auto ver = ver_from_index(ver_index);
    u32 offset = static_cast<u32>(ver.next());
    for (u32 it = 0; it < aux_index; ++it) {
      if (offset + sizeof(VerAux) > data->size()) return 0;
      offset += aux_from_offset(offset)->next;
    }
    return offset;
  }
  VerAux *aux_from_offset(u32 offset) const noexcept {
    if (offset + sizeof(VerAux) > data.size()) return {};
    return reinterpret_cast<const VerAux *>(data.data() + offset);
  }
  VerAux *aux_from_index(u32 need_index, u32 aux_index) const noexcept {
    auto offset = aux_offset_for_index(need_index, aux_index);
    return aux_from_offset(offset);
  }

  u32 add_ver(Ver &&needed) {
    auto data_pos = data->size();
    if (_vers > 0) {
      auto prev = ver_offset_for_index(_vers);
      // Update previous vd_next
      auto prev_needed = data->template get_at<Ver>(prev);
      prev_needed->set_next(data_pos - prev);
    }
    data->append(std::move(needed));
    _offsets_for_ver.emplace_back(data_pos);
    shdr.sh_size = data->size(), shdr.sh_info = ++_vers;
    return data_pos;
  }
  u32 add_aux(u32 need_index, VerAux &&aux) {
    if (_vers == 0) return 0;
    auto ver_offset = ver_offset_for_index(need_index);
    auto data_pos = data->size();
    Ver *ver = data->template get_at<Ver>(ver_offset);
    auto aux_offset = ver->aux();
    if (aux_offset != 0) {
      // Walk to the end of the aux chain
      u32 last_aux_offset = ver_offset + aux_offset;
      VerAux *last_aux = nullptr;
      while (last_aux_offset + sizeof(VerAux) <= data->size()) {
        last_aux = data->template get_at<VerAux>(last_aux_offset);
        if (last_aux->next() == 0) break;
        last_aux_offset += last_aux->next();
      }
      if (last_aux != nullptr) last_aux->set_next(data_pos - last_aux_offset);
    } else {
      // First aux for this needed entry
      auto offset = static_cast<u32>(data_pos - ver_offset);
      ver->set_aux(offset);
    }
    data->append(std::move(aux));
    shdr.sh_size = data->size();
    return data_pos;
  }

private:
  Shdr &shdr;
  Data &data;
  u32 _vers = 0;
  mutable std::vector<u32> _offsets_for_ver;
};

// Accessor for .gnu.version_r
template <ElfBits B, ElfEndian E, bool Const>
class PackedVersionNeedAccessor
    : public PackedVersionChainAccessor<B, E, Const, PackedElfVerneed<E>, PackedElfVernaux<E>> {
public:
  using Elf = maybe_const_t<Const, PackedElf<B, E>>;
  PackedVersionNeedAccessor(Elf &elf, u16 index)
      : PackedVersionChainAccessor<B, E, Const, PackedElfVerneed<E>, PackedElfVernaux<E>>(elf, index) {}
};
template <ElfBits B, ElfEndian E> using PackedVersionNeedReader = PackedVersionNeedAccessor<B, E, true>;
template <ElfBits B, ElfEndian E> using PackedVersionNeedWriter = PackedVersionNeedAccessor<B, E, false>;

// Accessor for .gnu.version_d
template <ElfBits B, ElfEndian E, bool Const>
class PackedVersionDefAccessor
    : public PackedVersionChainAccessor<B, E, Const, PackedElfVerdef<E>, PackedElfVerdaux<E>> {
public:
  using Elf = maybe_const_t<Const, PackedElf<B, E>>;
  PackedVersionDefAccessor(Elf &elf, u16 index)
      : PackedVersionChainAccessor<B, E, Const, PackedElfVerdef<E>, PackedElfVerdaux<E>>(elf, index) {}
};
template <ElfBits B, ElfEndian E> using PackedVersionDefReader = PackedVersionDefAccessor<B, E, true>;
template <ElfBits B, ElfEndian E> using PackedVersionDefWriter = PackedVersionDefAccessor<B, E, false>;

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
  return data_symtab->template get<PackedElfSymbol<B, E>>(index);
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
  data_symtab->append(std::move(symbol));
  shdr_symtab.sh_size = data_symtab->size();
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
    data->clear();
    data->allocate(dynamic_symtab.symbol_count() * shdr.sh_entsize, 0);
    shdr.sh_size = data->size();
  }
}

template <ElfBits B, ElfEndian E, bool Const>
u32 PackedSymbolVersionAccessor<B, E, Const>::version_count() const noexcept {
  return dynamic_symtab.symbol_count();
}

template <ElfBits B, ElfEndian E, bool Const>
u16 PackedSymbolVersionAccessor<B, E, Const>::get_version(u32 index) const noexcept {
  if (index >= version_count() || 1 + index * shdr.sh_entsize >= data->size()) return 0;
  return data->template get<U16<E>>(index);
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedSymbolVersionAccessor<B, E, Const>::set_version(u32 index, u16 version) noexcept {
  if (index >= version_count() || index * shdr.sh_entsize >= data->size()) return;
  *(data->template get<U16<E>>(index)) = version;
}

} // namespace pepp::bts
