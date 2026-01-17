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
#include "./packed_elf.hpp"
#include "bts/elf/packed_fixup.hpp"
namespace pepp::bts {
/*
 * A working dynamic section (e.g., decoded by readelf -d) requires a bunch of extra "things" beyond this class
 * - sh_link must point to a .dynstr string table
 * - A PT_DYNAMIC segment must exist, and it must contain this segment
 * - Segment sizes/offsets must be set correctly
 * - The main symbol table must contain a "_DYNAMIC" entry, and it must point to the start of the dynamic section in
 * memory.
 *
 * A good reference is for interpreting fields is:
 * https://refspecs.linuxfoundation.org/LSB_5.0.0/LSB-Core-generic/LSB-Core-generic/dynamicsection.html
 */
template <ElfBits B, ElfEndian E, bool Const> class PackedDynamicAccessor {
public:
  using Elf = maybe_const_t<Const, PackedElf<B, E>>;
  using Shdr = maybe_const_t<Const, PackedElfShdr<B, E>>;
  using Data = maybe_const_t<Const, std::shared_ptr<AStorage>>;
  PackedDynamicAccessor(Elf &elf, u16 index);
  PackedDynamicAccessor(Shdr &shdr, Data &data) noexcept;

  u32 entry_count() const noexcept;
  PackedElfDyn<B, E> get_entry(u32 index) const noexcept;
  PackedElfDyn<B, E> *get_entry_ptr(u32 index) const noexcept;
  u32 add_entry(PackedElfDyn<B, E> &&dyn);
  u32 add_entry(word<B> tag); // Add entry with 0 value. Often used for placeholders
  u32 add_entry(word<B> tag, word<B> value);
  u32 add_entry(DynamicTags tag); // Add entry with 0 value. Often used for placeholders
  u32 add_entry(DynamicTags tag, word<B> value);
  void replace_entry(u32 index, PackedElfDyn<B, E> &&dyn) noexcept;
  void replace_entry(u32 index, word<B> tag, word<B> value) noexcept;

private:
  Shdr &shdr;
  Data &data;
};
template <ElfBits B, ElfEndian E> using PackedDynamicReader = PackedDynamicAccessor<B, E, true>;
template <ElfBits B, ElfEndian E> using PackedDynamicWriter = PackedDynamicAccessor<B, E, false>;

template <ElfBits B, ElfEndian E>
AbsoluteFixup fixup_dynamic_value(PackedElf<B, E> &elf, u16 section, u32 index, std::function<word<B>()> func) {
  return AbsoluteFixup{.update = [elf, section, index, func]() {
    PackedDynamicReader<B, E> t(elf, section);
    auto ptr = t.get_entry_ptr(index);
    if (ptr != nullptr) ptr->d_val = func();
  }};
}

template <ElfBits B, ElfEndian E, bool Const>
PackedDynamicAccessor<B, E, Const>::PackedDynamicAccessor(Elf &elf, u16 index)
    : shdr(elf.section_headers[index]), data(elf.section_data[index]) {}

template <ElfBits B, ElfEndian E, bool Const>
PackedDynamicAccessor<B, E, Const>::PackedDynamicAccessor(Shdr &shdr, Data &data) noexcept : shdr(shdr), data(data) {}

template <ElfBits B, ElfEndian E, bool Const> u32 PackedDynamicAccessor<B, E, Const>::entry_count() const noexcept {
  if (shdr.sh_entsize == 0 || shdr.sh_size == 0) return 0;
  // Look for the first DT_NULL entry, otherwise clip to the number of entries computed from section header.
  auto max = shdr.sh_size / shdr.sh_entsize;
  for (u32 i = 0; i < max; ++i)
    if (auto dyn = get_entry(i); dyn.d_tag == to_underlying(DynamicTags::DT_NULL)) return i;
  return max;
}

template <ElfBits B, ElfEndian E, bool Const>
PackedElfDyn<B, E> PackedDynamicAccessor<B, E, Const>::get_entry(u32 index) const noexcept {
  if (auto ptr = get_entry_ptr(index); ptr != nullptr) return *ptr;
  return PackedElfDyn<B, E>{};
}

template <ElfBits B, ElfEndian E, bool Const>
PackedElfDyn<B, E> *PackedDynamicAccessor<B, E, Const>::get_entry_ptr(u32 index) const noexcept {
  if (index * shdr.sh_entsize + sizeof(PackedElfDyn<B, E>) > data->size()) return nullptr;
  return data->template get<PackedElfDyn<B, E>>(index);
}

template <ElfBits B, ElfEndian E, bool Const>
u32 PackedDynamicAccessor<B, E, Const>::add_entry(PackedElfDyn<B, E> &&dyn) {
  if (shdr.sh_entsize == 0) shdr.sh_entsize = sizeof(PackedElfDyn<B, E>);
  auto ret = entry_count();
  data->append(dyn);
  shdr.sh_size = data->size();
  return ret;
}

template <ElfBits B, ElfEndian E, bool Const> inline u32 PackedDynamicAccessor<B, E, Const>::add_entry(word<B> tag) {
  return add_entry(tag, 0);
}

template <ElfBits B, ElfEndian E, bool Const>
u32 PackedDynamicAccessor<B, E, Const>::add_entry(word<B> tag, word<B> value) {
  PackedElfDyn<B, E> dyn;
  dyn.d_tag = tag;
  dyn.d_val = value;
  return add_entry(std::move(dyn));
}

template <ElfBits B, ElfEndian E, bool Const> u32 PackedDynamicAccessor<B, E, Const>::add_entry(DynamicTags tag) {
  return add_entry(to_underlying(tag));
}

template <ElfBits B, ElfEndian E, bool Const>
u32 PackedDynamicAccessor<B, E, Const>::add_entry(DynamicTags tag, word<B> value) {
  return add_entry(to_underlying(tag), value);
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedDynamicAccessor<B, E, Const>::replace_entry(u32 index, PackedElfDyn<B, E> &&dyn) noexcept {
  if (auto ptr = get_entry_ptr(index); ptr != nullptr) {
    ptr->d_tag = dyn.d_tag;
    ptr->d_val = dyn.d_val;
  }
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedDynamicAccessor<B, E, Const>::replace_entry(u32 index, word<B> tag, word<B> value) noexcept {
  if (auto ptr = get_entry_ptr(index); ptr != nullptr) {
    ptr->d_tag = tag;
    ptr->d_val = value;
  }
}

} // namespace pepp::bts
