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
#include "core/formats/elf/packed_elf.hpp"
namespace pepp::bts {

// Convenience wrapper to "unpack" note entries for easier access.
struct NoteEntry {
  u32 namesz;
  u32 descsz;
  u32 type;
  bits::span<const char> name, desc;
};

// Accessor for ELF Note sections .
template <ElfBits B, ElfEndian E, bool Const> class PackedNoteAccessor {
public:
  using Elf = maybe_const_t<Const, PackedElf<B, E>>;
  using Shdr = maybe_const_t<Const, PackedElfShdr<B, E>>;
  using Data = maybe_const_t<Const, std::shared_ptr<AStorage>>;
  PackedNoteAccessor(Elf &elf, u16 index);
  PackedNoteAccessor(Shdr &note, Data &data) noexcept;
  static constexpr u64 round_up4(u64 n) { return ((n + 3) / 4) * 4; };

  u32 note_count() const noexcept;
  std::optional<NoteEntry> get_note(u32 index) const noexcept;
  void add_note(std::string_view name, std::string_view desc, u32 type);
  void add_note(std::span<const char> name, std::span<const char> desc, u32 type);

private:
  // Offset of the note following the one at `offset`.
  u64 next_offset(u64 offset) const noexcept;
  Shdr &shdr;
  Data &data;
};
template <ElfBits B, ElfEndian E> using PackedNoteReader = PackedNoteAccessor<B, E, true>;
template <ElfBits B, ElfEndian E> using PackedNoteWriter = PackedNoteAccessor<B, E, false>;

template <ElfBits B, ElfEndian E, bool Const>
PackedNoteAccessor<B, E, Const>::PackedNoteAccessor(PackedNoteAccessor<B, E, Const>::Elf &elf, u16 index)
    : shdr(checked_section_header(elf, index, "PackedNoteAccessor", "a note section", SectionTypes::SHT_NOTE)),
      data(elf.section_data[index]) {}

template <ElfBits B, ElfEndian E, bool Const>
PackedNoteAccessor<B, E, Const>::PackedNoteAccessor(Shdr &shdr_note, Data &data) noexcept
    : shdr(shdr_note), data(data) {}

template <ElfBits B, ElfEndian E, bool Const>
u64 PackedNoteAccessor<B, E, Const>::next_offset(u64 offset) const noexcept {
  const auto *hdr = data->template get_at<PackedElfNoteHeader<E>>(offset);
  return offset + sizeof(PackedElfNoteHeader<E>) + round_up4(hdr->n_namesz) + round_up4(hdr->n_descsz);
}

template <ElfBits B, ElfEndian E, bool Const> inline u32 PackedNoteAccessor<B, E, Const>::note_count() const noexcept {
  u32 count = 0;
  for (u64 offset = 0; offset + sizeof(PackedElfNoteHeader<E>) <= data->size(); offset = next_offset(offset)) count++;
  return count;
}

template <ElfBits B, ElfEndian E, bool Const>
std::optional<NoteEntry> PackedNoteAccessor<B, E, Const>::get_note(u32 index) const noexcept {
  u64 offset = 0;
  for (u32 it = 0; it < index && offset + sizeof(PackedElfNoteHeader<E>) <= data->size(); ++it)
    offset = next_offset(offset);
  if (offset + sizeof(PackedElfNoteHeader<E>) > data->size()) return std::nullopt;
  const auto *hdr = data->template get_at<PackedElfNoteHeader<E>>(offset);
  NoteEntry ret;
  ret.namesz = hdr->n_namesz;
  ret.descsz = hdr->n_descsz;
  ret.type = hdr->n_type;
  const u64 name_start = offset + sizeof(PackedElfNoteHeader<E>), desc_start = name_start + round_up4(ret.namesz);
  const auto name = data->get(name_start, ret.namesz), desc = data->get(desc_start, ret.descsz);
  if (name.size() != ret.namesz || desc.size() != ret.descsz) return std::nullopt;
  ret.name = bits::span<const char>(reinterpret_cast<const char *>(name.data()), name.size());
  ret.desc = bits::span<const char>(reinterpret_cast<const char *>(desc.data()), desc.size());
  return ret;
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedNoteAccessor<B, E, Const>::add_note(std::string_view name, std::string_view desc, u32 type) {
  add_note(std::span<const char>(name), std::span<const char>(desc), type);
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedNoteAccessor<B, E, Const>::add_note(std::span<const char> name, std::span<const char> desc, u32 type) {
  u32 namesz = name.size(), descsz = desc.size();
  if (name.back() != 0) namesz++;
  PackedElfNoteHeader<E> hdr(namesz, descsz, type);
  u64 hdr_start = data->size(), name_start = hdr_start + sizeof(PackedElfNoteHeader<E>),
      desc_start = name_start + round_up4(namesz);
  auto start = data->allocate(desc_start + round_up4(descsz) - hdr_start, 0);
  data->set(start, std::move(hdr));
  data->set(name_start, bits::span<const u8>{(const u8 *)name.data(), name.size()});
  data->set(desc_start, bits::span<const u8>{(const u8 *)desc.data(), desc.size()});
  shdr.sh_size = data->size();
}
} // namespace pepp::bts
