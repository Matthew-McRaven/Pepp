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
// Accessor for ELF string tables
template <ElfBits B, ElfEndian E, bool Const> class PackedStringAccessor {
public:
  using Elf = maybe_const_t<Const, PackedElf<B, E>>;
  using Shdr = maybe_const_t<Const, PackedElfShdr<B, E>>;
  using Data = maybe_const_t<Const, std::shared_ptr<AStorage>>;
  PackedStringAccessor(Elf &elf, u16 index);
  PackedStringAccessor(Shdr &shdr, Data &strtab) noexcept;
  const char *get_string(word<B> index) const noexcept;
  bits::span<const char> get_string_span(word<B> index) const noexcept;
  word<B> find(std::string_view) const noexcept;
  // Returns the index to the start of the added string
  word<B> add_string(std::span<const char> str);
  word<B> add_string(const char *str);
  word<B> add_string(std::string_view str);
  word<B> add_string(const std::string &str);

private:
  Shdr &shdr;
  Data &strtab;
};
template <ElfBits B, ElfEndian E> using PackedStringReader = PackedStringAccessor<B, E, true>;
template <ElfBits B, ElfEndian E> using PackedStringWriter = PackedStringAccessor<B, E, false>;

template <ElfBits B, ElfEndian E, bool Const>
PackedStringAccessor<B, E, Const>::PackedStringAccessor(PackedStringAccessor<B, E, Const>::Elf &elf, u16 index)
    : shdr(elf.section_headers[index]), strtab(elf.section_data[index]) {
  if (index > elf.section_headers.size()) throw std::runtime_error("PackedStringReader: invalid section index");
}

template <ElfBits B, ElfEndian E, bool Const>
PackedStringAccessor<B, E, Const>::PackedStringAccessor(PackedStringAccessor<B, E, Const>::Shdr &shdr,
                                                        PackedStringAccessor<B, E, Const>::Data &strtab) noexcept
    : shdr(shdr), strtab(strtab) {}

template <ElfBits B, ElfEndian E, bool Const>
const char *PackedStringAccessor<B, E, Const>::get_string(word<B> index) const noexcept {
  if (index >= shdr.sh_size || index >= strtab.size()) return nullptr;
  return reinterpret_cast<const char *>(strtab.data() + index);
}

template <ElfBits B, ElfEndian E, bool Const>
bits::span<const char> PackedStringAccessor<B, E, Const>::get_string_span(word<B> index) const noexcept {
  if (index >= shdr.sh_size || index >= strtab->size()) return {};
  auto sp = strtab->get(index, strtab->strlen(index));
  return bits::span<const char>{(const char *)sp.data(), sp.size()};
}

template <ElfBits B, ElfEndian E, bool Const>
word<B> PackedStringAccessor<B, E, Const>::find(std::string_view needle) const noexcept {
  if (needle.empty()) return 0;
  return strtab->find(bits::span<const u8>{(const u8 *)needle.data(), needle.size()});
}

template <ElfBits B, ElfEndian E, bool Const>
word<B> PackedStringAccessor<B, E, Const>::add_string(std::span<const char> str) {
  // Ensure the first character is always null
  if (strtab->size() == 0) strtab->template append<u8>(0);
  const word<B> new_size = str.size() + (str.back() != '\0' ? 1 : 0);
  const word<B> ret = strtab->allocate(new_size);
  strtab->set(ret, std::span<const u8>{(const u8 *)str.data(), str.size()});
  if (str.back() != '\0') strtab->template set<u8>(ret + str.size(), 0);
  shdr.sh_size = strtab->size();
  return ret;
}

template <ElfBits B, ElfEndian E, bool Const> word<B> PackedStringAccessor<B, E, Const>::add_string(const char *str) {
  if (!str) return 0;
  auto len = std::strlen(str) + 1;
  return add_string(bits::span<const char>(str, len));
}

template <ElfBits B, ElfEndian E, bool Const>
word<B> PackedStringAccessor<B, E, Const>::add_string(std::string_view str) {
  return add_string(std::span{str});
}

template <ElfBits B, ElfEndian E, bool Const>
word<B> PackedStringAccessor<B, E, Const>::add_string(const std::string &str) {
  return add_string(std::span{str});
}

} // namespace pepp::bts
