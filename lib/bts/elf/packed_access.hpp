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

template <ElfBits B, ElfEndian E> class PackedSymbolReader {
public:
  using PackedElfSymbol = PackedElfSymbol<B, E>;
  PackedSymbolReader(const PackedElf<B, E> &elf, u16 index);
  PackedSymbolReader(const PackedElfShdr<B, E> &shdr_symbol, const std::vector<u8> &symtab,
                     const PackedElfShdr<B, E> &shdr, const std::vector<u8> &strtab) noexcept;

  u32 symbol_count() const noexcept;
  PackedElfSymbol get_symbol(u32 index) const noexcept;
  std::optional<PackedElfSymbol> find_symbol(std::string_view name) const noexcept;
  std::optional<PackedElfSymbol> find_symbol(Word<B, E> address) const noexcept;

private:
  const PackedElfShdr<B, E> &shdr;
  const std::vector<u8> &symtab;
  PackedStringReader<B, E> strtab;
};

template <ElfBits B, ElfEndian E> class PackedSymbolWriter {
public:
  using PackedElfSymbol = PackedElfSymbol<B, E>;
  PackedSymbolWriter(PackedElf<B, E> &elf, u16 index);
  PackedSymbolWriter(PackedElfShdr<B, E> &shdr_symbol, std::vector<u8> &symtab, PackedElfShdr<B, E> &shdr,
                     std::vector<u8> &strtab) noexcept;

  u32 symbol_count() const noexcept;
  PackedElfSymbol get_symbol(u32 index) const noexcept;
  PackedElfSymbol *get_symbol_ptr(u32 index) const noexcept;
  std::optional<PackedElfSymbol> find_symbol(std::string_view name) const noexcept;
  std::optional<PackedElfSymbol> find_symbol(Word<B, E> address) const noexcept;

  // Add a symbol to the table. Assumes you already set st_name!
  void add_symbol(PackedElfSymbol &&symbol);
  // Helpers to assign name and section index.
  void add_symbol(PackedElfSymbol &&symbol, std::string_view name);
  void add_symbol(PackedElfSymbol &&symbol, std::string_view name, u16 section_index);
  void arrange_local_symbols(std::function<void(Word<B, E> first, Word<B, E> second)> func = nullptr);

private:
  void copy_to_symtab(PackedElfSymbol &&symbol);
  PackedElfShdr<B, E> &shdr;
  std::vector<u8> &symtab;
  PackedStringWriter<B, E> strtab;
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

// Symbols
template <ElfBits B, ElfEndian E>
PackedSymbolReader<B, E>::PackedSymbolReader(const PackedElf<B, E> &elf, u16 index)
    : shdr(elf.section_headers[index]), symtab(elf.section_data[index]), strtab(elf, shdr.sh_link) {
  if (index > elf.section_headers.size()) throw std::runtime_error("PackedSymbolReader: invalid section index");
}

template <ElfBits B, ElfEndian E>
PackedSymbolReader<B, E>::PackedSymbolReader(const PackedElfShdr<B, E> &shdr_symbol, const std::vector<u8> &symtab,
                                             const PackedElfShdr<B, E> &shdr, const std::vector<u8> &strtab) noexcept
    : shdr(shdr_symbol), symtab(symtab), strtab(shdr, strtab) {}

template <ElfBits B, ElfEndian E> u32 PackedSymbolReader<B, E>::symbol_count() const noexcept {
  if (shdr.sh_entsize == 0 || shdr.sh_size == 0) return 0;
  return shdr.sh_size / shdr.sh_entsize;
}

template <ElfBits B, ElfEndian E> PackedElfSymbol<B, E> PackedSymbolReader<B, E>::get_symbol(u32 index) const noexcept {
  if (index >= symbol_count()) return PackedElfSymbol{};
  const u8 *ptr = symtab.data() + index * shdr.sh_entsize;
  // Use memcpy to avoid unaligned access issues
  PackedElfSymbol ret;
  memcpy(&ret, ptr, sizeof(PackedElfSymbol));
  return ret;
}

template <ElfBits B, ElfEndian E>
std::optional<PackedElfSymbol<B, E>> PackedSymbolReader<B, E>::find_symbol(std::string_view name) const noexcept {
  // TODO: if there is a hash table index, use it for faster lookup.
  auto str_idx = strtab.find(name);
  if (str_idx == 0 && !name.empty()) return std::nullopt;
  for (u32 it = 0; it < symbol_count(); ++it)
    if (auto sym = get_symbol(it); sym.st_name == str_idx) return sym;
  return std::nullopt;
}

template <ElfBits B, ElfEndian E>
std::optional<PackedElfSymbol<B, E>> PackedSymbolReader<B, E>::find_symbol(Word<B, E> address) const noexcept {
  for (u32 it = 0; it < symbol_count(); ++it)
    if (auto sym = get_symbol(it); sym.st_value == address) return sym;
  return std::nullopt;
}

template <ElfBits B, ElfEndian E>
PackedSymbolWriter<B, E>::PackedSymbolWriter(PackedElf<B, E> &elf, u16 index)
    : shdr(elf.section_headers[index]), symtab(elf.section_data[index]), strtab(elf, shdr.sh_link) {
  if (index > elf.section_headers.size()) throw std::runtime_error("PackedSymbolWriter: invalid section index");
}

template <ElfBits B, ElfEndian E>
PackedSymbolWriter<B, E>::PackedSymbolWriter(PackedElfShdr<B, E> &shdr_symbol, std::vector<u8> &symtab,
                                             PackedElfShdr<B, E> &shdr, std::vector<u8> &strtab) noexcept
    : shdr(shdr_symbol), symtab(symtab), strtab(shdr, strtab) {}

template <ElfBits B, ElfEndian E> u32 PackedSymbolWriter<B, E>::symbol_count() const noexcept {
  if (shdr.sh_entsize == 0 || shdr.sh_size == 0) return 0;
  return shdr.sh_size / shdr.sh_entsize;
}

template <ElfBits B, ElfEndian E> PackedElfSymbol<B, E> PackedSymbolWriter<B, E>::get_symbol(u32 index) const noexcept {
  auto ptr = get_symbol_ptr(index);
  if (ptr == nullptr) return {};
  return *ptr;
}

template <ElfBits B, ElfEndian E>
PackedElfSymbol<B, E> *PackedSymbolWriter<B, E>::get_symbol_ptr(u32 index) const noexcept {
  if (index >= symbol_count()) return nullptr;
  auto ret = (PackedElfSymbol *)(symtab.data() + index * shdr.sh_entsize);
  return ret;
}

template <ElfBits B, ElfEndian E>
std::optional<PackedElfSymbol<B, E>> PackedSymbolWriter<B, E>::find_symbol(std::string_view name) const noexcept {
  // TODO: if there is a hash table index, use it for faster lookup.
  auto str_idx = strtab.find(name);
  if (str_idx == 0 && !name.empty()) return std::nullopt;
  for (u32 it = 0; it < symbol_count(); ++it)
    if (auto sym = get_symbol(it); sym.st_name == str_idx) return sym;
  return std::nullopt;
}

template <ElfBits B, ElfEndian E>
std::optional<PackedElfSymbol<B, E>> PackedSymbolWriter<B, E>::find_symbol(Word<B, E> address) const noexcept {
  for (u32 it = 0; it < symbol_count(); ++it)
    if (auto sym = get_symbol(it); sym.st_value == address) return sym;
  return std::nullopt;
}

template <ElfBits B, ElfEndian E> void PackedSymbolWriter<B, E>::add_symbol(PackedElfSymbol &&symbol) {
  if (shdr.sh_size == 0) copy_to_symtab(create_null_symbol<B, E>());
  copy_to_symtab(std::move(symbol));
}

template <ElfBits B, ElfEndian E>
void PackedSymbolWriter<B, E>::add_symbol(PackedElfSymbol &&symbol, std::string_view name) {
  if (shdr.sh_size == 0) copy_to_symtab(create_null_symbol<B, E>());
  add_symbol(std::move(symbol), name, symbol.st_shndx);
}

template <ElfBits B, ElfEndian E>
void PackedSymbolWriter<B, E>::add_symbol(PackedElfSymbol &&symbol, std::string_view name, u16 section_index) {
  if (shdr.sh_size == 0) copy_to_symtab(create_null_symbol<B, E>());
  auto name_idx = strtab.add_string(name);
  symbol.st_name = name_idx;
  symbol.st_shndx = section_index;
  copy_to_symtab(std::move(symbol));
}

template <ElfBits B, ElfEndian E>
void PackedSymbolWriter<B, E>::arrange_local_symbols(std::function<void(Word<B, E>, Word<B, E>)> func) {
  u32 first_not_local = 1; // Skip the first entry. It is always NOTYPE
  word<B> current = 0;
  word<B> count = symbol_count();

  while (true) {
    PackedElfSymbol *p1 = nullptr;
    PackedElfSymbol *p2 = nullptr;

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
      std::swap(*p1, *p2);
    } else {
      // Update 'info' field of the section
      shdr.sh_info = first_not_local;
      break;
    }
  }
}

template <ElfBits B, ElfEndian E> void PackedSymbolWriter<B, E>::copy_to_symtab(PackedElfSymbol &&symbol) {
  if (shdr.sh_entsize == 0) shdr.sh_entsize = sizeof(PackedElfSymbol);
  const u8 *ptr = reinterpret_cast<const u8 *>(&symbol);
  symtab.insert(symtab.end(), ptr, ptr + sizeof(PackedElfSymbol));
  shdr.sh_size += sizeof(PackedElfSymbol);
}

} // namespace pepp::bts
