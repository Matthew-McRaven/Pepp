#pragma once
#include "./packed_elf.hpp"
namespace pepp::bts {
template <ElfBits B, ElfEndian E, bool Const> class PackedStringAccessor {
public:
  using Elf = maybe_const_t<Const, PackedElf<B, E>>;
  using Shdr = maybe_const_t<Const, PackedElfShdr<B, E>>;
  using Data = maybe_const_t<Const, std::vector<u8>>;
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
  if (index >= shdr.sh_size || index >= strtab.size()) return {};
  const char *start = (const char *)strtab.data() + index, *end = start;
  while (end < (const char *)strtab.data() + strtab.size() && *end != '\0') ++end;
  return bits::span<const char>(start, end - start);
}

template <ElfBits B, ElfEndian E, bool Const>
word<B> PackedStringAccessor<B, E, Const>::find(std::string_view needle) const noexcept {
  if (needle.empty()) return 0;
  auto it = std::search(strtab.begin(), strtab.end(), needle.begin(), needle.end());
  return (it == strtab.end()) ? 0 : static_cast<std::size_t>(it - strtab.begin());
}

template <ElfBits B, ElfEndian E, bool Const>
word<B> PackedStringAccessor<B, E, Const>::add_string(std::span<const char> str) {
  // Ensure the first character is always null
  if (strtab.size() == 0) strtab.push_back('\0');
  // Strings are addeded to the end of the current section data
  word<B> ret = strtab.size();
  strtab.insert(strtab.end(), str.begin(), str.end());
  if (strtab.back() != '\0') strtab.push_back('\0');
  shdr.sh_size = strtab.size();
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
