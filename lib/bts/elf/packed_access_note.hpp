#pragma once
#include "./packed_elf.hpp"
namespace pepp::bts {
struct NoteEntry {
  u32 namesz;
  u32 descsz;
  u32 type;
  bits::span<const char> name, desc;
};
template <ElfBits B, ElfEndian E, bool Const> class PackedNoteAccessor {
public:
  using Elf = maybe_const_t<Const, PackedElf<B, E>>;
  using Shdr = maybe_const_t<Const, PackedElfShdr<B, E>>;
  using Data = maybe_const_t<Const, std::vector<u8>>;
  PackedNoteAccessor(Elf &elf, u16 index);
  PackedNoteAccessor(Shdr &note, Data &data) noexcept;
  static constexpr u64 round_up4(u64 n) { return ((n + 3) / 4) * 4; };

  u32 note_count() const noexcept;
  std::optional<NoteEntry> get_note(u32 index) const noexcept;
  void add_note(std::string_view name, std::string_view desc, u32 type);
  void add_note(std::span<const char> name, std::span<const char> desc, u32 type);

private:
  Shdr &shdr;
  Data &data;
};
template <ElfBits B, ElfEndian E> using PackedNoteReader = PackedNoteAccessor<B, E, true>;
template <ElfBits B, ElfEndian E> using PackedNoteWriter = PackedNoteAccessor<B, E, false>;

template <ElfBits B, ElfEndian E, bool Const>
PackedNoteAccessor<B, E, Const>::PackedNoteAccessor(PackedNoteAccessor<B, E, Const>::Elf &elf, u16 index)
    : shdr(elf.section_headers[index]), data(elf.section_data[index]) {
  if (index > elf.section_headers.size()) throw std::runtime_error("PackedNoteAccessor: invalid section index");
}

template <ElfBits B, ElfEndian E, bool Const>
PackedNoteAccessor<B, E, Const>::PackedNoteAccessor(Shdr &shdr_note, Data &data) noexcept
    : shdr(shdr_note), data(data) {}

template <ElfBits B, ElfEndian E, bool Const> inline u32 PackedNoteAccessor<B, E, Const>::note_count() const noexcept {
  u32 current_pos = 0, current_idx = 0;
  PackedElfNoteHeader<E> *hdr = nullptr;
  while (current_pos < data.size()) {
    hdr = reinterpret_cast<const PackedElfNoteHeader<E> *>(data.data() + current_pos);
    current_pos += sizeof(PackedElfNoteHeader<E>) + round_up4(hdr->n_namesz) + round_up4(hdr->n_descsz);
    current_idx++;
  }
  return current_idx;
}

template <ElfBits B, ElfEndian E, bool Const>
std::optional<NoteEntry> PackedNoteAccessor<B, E, Const>::get_note(u32 index) const noexcept {
  u32 current_pos = 0, current_idx = 0;
  PackedElfNoteHeader<E> *hdr = nullptr;
  while (current_idx != index && current_pos < data.size()) {
    hdr = reinterpret_cast<const PackedElfNoteHeader<E> *>(data.data() + current_pos);
    current_pos += sizeof(PackedElfNoteHeader<E>) + round_up4(hdr->n_namesz) + round_up4(hdr->n_descsz);
    current_idx++;
  }
  if (current_pos >= data.size() || current_idx != index) return std::nullopt;
  NoteEntry ret;
  ret.namesz = hdr->n_namesz;
  ret.descsz = hdr->n_descsz;
  ret.type = hdr->n_type;
  ret.name = bits::span<const char>(reinterpret_cast<const char *>(hdr + 1), hdr->n_namesz);
  ret.desc = bits::span<const char>(
      reinterpret_cast<const char *>(reinterpret_cast<const u8 *>(hdr + 1) + round_up4(hdr->n_namesz)), hdr->n_descsz);
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
  data.insert(data.end(), reinterpret_cast<const u8 *>(&hdr),
              reinterpret_cast<const u8 *>(&hdr) + sizeof(PackedElfNoteHeader<E>));
  data.insert(data.end(), name.begin(), name.end());
  if (name.back() != 0) data.push_back('\0');
  while (data.size() % 4 != 0) data.push_back('\0');
  data.insert(data.end(), desc.begin(), desc.end());
  while (data.size() % 4 != 0) data.push_back('\0');
  shdr.sh_size = data.size();
}
} // namespace pepp::bts
