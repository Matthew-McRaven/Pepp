#pragma once
#include "./packed_elf.hpp"
namespace pepp::bts {

template <ElfBits B, ElfEndian E> class PackedStringReader {
public:
  PackedStringReader(const PackedElf<B, E> &elf, u16 index);
  PackedStringReader(const PackedElfShdr<B, E> &shdr, const std::vector<u8> &strtab) noexcept;
  const char *get_string(word<B> index) const noexcept;
  word<B> find(std::string_view) const noexcept;

private:
  const PackedElfShdr<B, E> &shdr;
  const std::vector<u8> &strtab;
};

template <ElfBits B, ElfEndian E> class PackedStringWriter {
public:
  PackedStringWriter(PackedElf<B, E> &elf, u16 index);
  PackedStringWriter(PackedElfShdr<B, E> &shdr, std::vector<u8> &strtab) noexcept;
  // PackedStringRead API
  const char *get_string(word<B> index) const noexcept;
  word<B> find(std::string_view) const noexcept;
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

template <ElfBits B, ElfEndian E>
PackedStringReader<B, E>::PackedStringReader(const PackedElf<B, E> &elf, u16 index)
    : shdr(elf.section_headers[index]), strtab(elf.section_data[index]) {
  if (index > elf.section_headers.size()) throw std::runtime_error("PackedStringReader: invalid section index");
}

template <ElfBits B, ElfEndian E>
PackedStringReader<B, E>::PackedStringReader(const PackedElfShdr<B, E> &shdr, const std::vector<u8> &strtab) noexcept
    : shdr(shdr), strtab(strtab) {}

template <ElfBits B, ElfEndian E> const char *PackedStringReader<B, E>::get_string(word<B> index) const noexcept {
  if (index >= shdr.sh_size || index >= strtab.size()) return nullptr;
  return reinterpret_cast<const char *>(strtab.data() + index);
}

template <ElfBits B, ElfEndian E> word<B> PackedStringReader<B, E>::find(std::string_view needle) const noexcept {
  if (needle.empty()) return 0;
  auto it = std::search(strtab.begin(), strtab.end(), needle.begin(), needle.end());
  return (it == strtab.end()) ? 0 : static_cast<std::size_t>(it - strtab.begin());
}

template <ElfBits B, ElfEndian E>
PackedStringWriter<B, E>::PackedStringWriter(PackedElfShdr<B, E> &shdr, std::vector<u8> &strtab) noexcept
    : shdr(shdr), strtab(strtab) {}

template <ElfBits B, ElfEndian E>
PackedStringWriter<B, E>::PackedStringWriter(PackedElf<B, E> &elf, u16 index)
    : shdr(elf.section_headers[index]), strtab(elf.section_data[index]) {
  if (index > elf.section_headers.size()) throw std::runtime_error("PackedStringReader: invalid section index");
}

template <ElfBits B, ElfEndian E> const char *PackedStringWriter<B, E>::get_string(word<B> index) const noexcept {
  if (index >= shdr.sh_size || index >= strtab.size()) return nullptr;
  return reinterpret_cast<const char *>(strtab.data() + index);
}

template <ElfBits B, ElfEndian E> word<B> PackedStringWriter<B, E>::find(std::string_view needle) const noexcept {
  if (needle.empty()) return 0;
  auto it = std::search(strtab.begin(), strtab.end(), needle.begin(), needle.end());
  return (it == strtab.end()) ? 0 : static_cast<std::size_t>(it - strtab.begin());
}

template <ElfBits B, ElfEndian E> word<B> PackedStringWriter<B, E>::add_string(std::span<const char> str) {
  // Ensure the first character is always null
  if (strtab.size() == 0) strtab.push_back('\0');
  // Strings are addeded to the end of the current section data
  word<B> ret = strtab.size();
  strtab.insert(strtab.end(), str.begin(), str.end());
  if (strtab.back() != '\0') strtab.push_back('\0');
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
