#pragma once
#include "./packed_elf.hpp"
namespace pepp::bts {

/*
 * Used to access .init_array, .fini_array, etc. These are effectively arrays of function pointers.
 */
template <ElfBits B, ElfEndian E, bool Const> class PackedArrayAccessor {
public:
  using Elf = maybe_const_t<Const, PackedElf<B, E>>;
  using Shdr = maybe_const_t<Const, PackedElfShdr<B, E>>;
  using Data = maybe_const_t<Const, std::vector<u8>>;
  PackedArrayAccessor(Elf &elf, u16 index);
  PackedArrayAccessor(Shdr &shdr, Data &data) noexcept;

  u32 entry_count() const noexcept;
  word<B> get_entry(u32 index) const noexcept;
  void add_entry(word<B>);

private:
  Shdr &shdr;
  Data &data;
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
  return *(((Word<B, E> *)data.data()) + index);
}

template <ElfBits B, ElfEndian E, bool Const> void PackedArrayAccessor<B, E, Const>::add_entry(word<B> address) {
  if (shdr.sh_entsize == 0) shdr.sh_entsize = sizeof(Word<B, E>);
  Word<B, E> adjust = address;
  auto ate = data.size();
  data.resize(ate + sizeof(Word<B, E>));
  std::memcpy(data.data() + ate, &adjust, sizeof(Word<B, E>));
  shdr.sh_size = data.size();
}

} // namespace pepp::bts
