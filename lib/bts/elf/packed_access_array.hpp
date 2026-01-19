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
#include "bts/elf/packed_elf.hpp"
namespace pepp::bts {

/*
 * Used to access .init_array, .fini_array, etc. These are effectively arrays of function pointers.
 */
template <ElfBits B, ElfEndian E, bool Const> class PackedArrayAccessor {
public:
  using Elf = maybe_const_t<Const, PackedElf<B, E>>;
  using Shdr = maybe_const_t<Const, PackedElfShdr<B, E>>;
  using Data = maybe_const_t<Const, std::shared_ptr<AStorage>>;
  PackedArrayAccessor(Elf &elf, u16 index);
  PackedArrayAccessor(Shdr &shdr, Data &data) noexcept;

  u32 entry_count() const noexcept;
  word<B> get_entry(u32 index) const noexcept;
  void add_entry(word<B>);

private:
  Shdr &shdr;
  Data data;
};
template <ElfBits B, ElfEndian E> using PackedArrayReader = PackedArrayAccessor<B, E, true>;
template <ElfBits B, ElfEndian E> using PackedArrayWriter = PackedArrayAccessor<B, E, false>;

template <ElfBits B, ElfEndian E, bool Const>
PackedArrayAccessor<B, E, Const>::PackedArrayAccessor(Elf &elf, u16 index)
    : shdr(elf.section_headers[index]), data(elf.section_data[index]) {}

template <ElfBits B, ElfEndian E, bool Const>
PackedArrayAccessor<B, E, Const>::PackedArrayAccessor(Shdr &shdr, Data &data) noexcept : shdr(shdr), data(data) {}

template <ElfBits B, ElfEndian E, bool Const> u32 PackedArrayAccessor<B, E, Const>::entry_count() const noexcept {
  if (shdr.sh_entsize == 0 || shdr.sh_size == 0) return 0;
  return shdr.sh_size / shdr.sh_entsize;
}

template <ElfBits B, ElfEndian E, bool Const>
word<B> PackedArrayAccessor<B, E, Const>::get_entry(u32 index) const noexcept {
  if (index * sizeof(Word<B, E>) >= data.size()) return 0;
  return data->template get<Word<B, E>>(index);
}

template <ElfBits B, ElfEndian E, bool Const> void PackedArrayAccessor<B, E, Const>::add_entry(word<B> address) {
  if (shdr.sh_entsize == 0) shdr.sh_entsize = sizeof(Word<B, E>);
  Word<B, E> adjust = address;
  data->append(adjust);
  shdr.sh_size = data->size();
}

} // namespace pepp::bts
