#pragma once
#include "./packed_elf.hpp"
namespace pepp::bts {

template <ElfBits B, ElfEndian E> class PackedStringReader {
public:
  PackedStringReader(const PackedElf<B, E> &elf, u16 index);
  PackedStringReader(const PackedElfShdr<B, E> &shdr, const std::vector<u8> &strtab) noexcept;
  const char *get_string(word<B> index) const noexcept;

private:
  const PackedElfShdr<B, E> &shdr;
  const std::vector<u8> &strtab;
};

template <ElfBits B, ElfEndian E> class PackedStringWriter {
public:
  PackedStringWriter(PackedElf<B, E> &elf, u16 index);
  PackedStringWriter(PackedElfShdr<B, E> &shdr, std::vector<u8> &strtab) noexcept;
  const char *get_string(word<B> index) const noexcept;
  // Returns the index to the start of the added string
  word<B> add_string(std::span<const char> str);
  word<B> add_string(const char *str);
  word<B> add_string(std::string_view str);
  word<B> add_string(const std::string &str);

private:
  PackedElfShdr<B, E> &shdr;
  std::vector<u8> &strtab;
};

// Strings

template <ElfBits B, ElfEndian E> PackedStringReader<B, E>::PackedStringReader(const PackedElf<B, E> &elf, u16 index) {
  if (index > elf.section_headers.size()) throw std::runtime_error("PackedStringReader: invalid section index");
  shdr = elf.section_headers[index];
  strtab = elf.section_data[index];
}

template <ElfBits B, ElfEndian E>
PackedStringReader<B, E>::PackedStringReader(const PackedElfShdr<B, E> &shdr, const std::vector<u8> &strtab) noexcept
    : shdr(shdr), strtab(strtab) {}

template <ElfBits B, ElfEndian E> const char *PackedStringReader<B, E>::get_string(word<B> index) const noexcept {
  if (index >= shdr.sh_size || index >= strtab.size()) return nullptr;
  return reinterpret_cast<const char *>(strtab.data() + index);
}

template <ElfBits B, ElfEndian E>
PackedStringWriter<B, E>::PackedStringWriter(PackedElfShdr<B, E> &shdr, std::vector<u8> &strtab) noexcept
    : shdr(shdr), strtab(strtab) {}

template <ElfBits B, ElfEndian E> PackedStringWriter<B, E>::PackedStringWriter(PackedElf<B, E> &elf, u16 index) {
  if (index > elf.section_headers.size()) throw std::runtime_error("PackedStringReader: invalid section index");
  shdr = elf.section_headers[index];
  strtab = elf.section_data[index];
}

template <ElfBits B, ElfEndian E> const char *PackedStringWriter<B, E>::get_string(word<B> index) const noexcept {
  if (index >= shdr.sh_size || index >= strtab.size()) return nullptr;
  return reinterpret_cast<const char *>(strtab.data() + index);
}

template <ElfBits B, ElfEndian E> word<B> PackedStringWriter<B, E>::add_string(std::span<const char> str) {
  // Ensure the first character is always null
  if (strtab.size() == 0) strtab.push_back('\0');
  // Strings are addeded to the end of the current section data
  word<B> ret = strtab.size();
  strtab.insert(strtab.end(), str.begin(), str.end());
  return ret;
}

template <ElfBits B, ElfEndian E> word<B> PackedStringWriter<B, E>::add_string(const char *str) {
  if (!str) return 0;
  auto len = std::strlen(str) + 1;
  return add_string(bits::span<const char>(str, len));
}

template <ElfBits B, ElfEndian E> word<B> PackedStringWriter<B, E>::add_string(std::string_view str) {
  return add_string(std::span{str});
}

template <ElfBits B, ElfEndian E> word<B> PackedStringWriter<B, E>::add_string(const std::string &str) {
  return add_string(std::span{str});
}

} // namespace pepp::bts
