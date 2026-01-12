#pragma once
#include "./packed_elf.hpp"
namespace pepp::bts {

template <bool Const, class T> using maybe_const_t = std::conditional_t<Const, T const, T>;
template <ElfBits B, ElfEndian E, bool Const> class PackedStringAccessor {
public:
  using Elf = maybe_const_t<Const, PackedElf<B, E>>;
  using Shdr = maybe_const_t<Const, PackedElfShdr<B, E>>;
  using Data = maybe_const_t<Const, std::vector<u8>>;
  PackedStringAccessor(Elf &elf, u16 index);
  PackedStringAccessor(Shdr &shdr, Data &strtab) noexcept;
  const char *get_string(word<B> index) const noexcept;
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

template <ElfBits B, ElfEndian E, bool Const> class PackedSymbolAccessor {
public:
  using Elf = maybe_const_t<Const, PackedElf<B, E>>;
  using Shdr = maybe_const_t<Const, PackedElfShdr<B, E>>;
  using Data = maybe_const_t<Const, std::vector<u8>>;

  PackedSymbolAccessor(Elf &elf, u16 index);
  PackedSymbolAccessor(Shdr &shdr_symbol, Data &symtab, Shdr &shdr, Data &strtab) noexcept;

  u32 symbol_count() const noexcept;
  PackedElfSymbol<B, E> get_symbol(u32 index) const noexcept;
  PackedElfSymbol<B, E> *get_symbol_ptr(u32 index) const noexcept;
  u32 find_symbol_index(std::string_view name) const noexcept;
  std::optional<PackedElfSymbol<B, E>> find_symbol(std::string_view name) const noexcept;
  std::optional<PackedElfSymbol<B, E>> find_symbol(Word<B, E> address) const noexcept;
  void replace_value(u32 index, Word<B, E> value) noexcept;

  // Add a symbol to the table. Assumes you already set st_name!
  u32 add_symbol(PackedElfSymbol<B, E> &&symbol);
  // Helpers to assign name and section index.
  u32 add_symbol(PackedElfSymbol<B, E> &&symbol, std::string_view name);
  u32 add_symbol(PackedElfSymbol<B, E> &&symbol, std::string_view name, u16 section_index);
  void arrange_local_symbols(std::function<void(Word<B, E> first, Word<B, E> second)> func = nullptr);

private:
  void copy_to_symtab(PackedElfSymbol<B, E> &&symbol);
  Shdr &shdr;
  Data &symtab;
  PackedStringAccessor<B, E, Const> strtab;
};
template <ElfBits B, ElfEndian E> using PackedSymbolReader = PackedSymbolAccessor<B, E, true>;
template <ElfBits B, ElfEndian E> using PackedSymbolWriter = PackedSymbolAccessor<B, E, false>;

// Handles both REL and RELA.
// You just need to know which one you're dealing with.
template <ElfBits B, ElfEndian E, bool Const> class PackedRelocationAccessor {
public:
  using Elf = maybe_const_t<Const, PackedElf<B, E>>;
  using Shdr = maybe_const_t<Const, PackedElfShdr<B, E>>;
  using Data = maybe_const_t<Const, std::vector<u8>>;

  PackedRelocationAccessor(Elf &elf, u16 index);
  PackedRelocationAccessor(Shdr &shdr_reloc, Data &reloc, const PackedElfShdr<B, E> &shdr_symbol,
                           const std::vector<u8> &symtab, const PackedElfShdr<B, E> &shdr_strtab,
                           std::vector<u8> &strtab) noexcept;
  bool is_rela() const noexcept;
  u32 relocation_count() const noexcept;
  PackedElfRel<B, E> get_rel(u32 index) const noexcept;
  PackedElfRelA<B, E> get_rela(u32 index) const noexcept;

  void add_rel(PackedElfRel<B, E> &&rel);
  void add_rel(word<B> offset, u32 type, u32 symbol);
  void add_rel(word<B> offset, u32 type, std::string_view name);
  void replace_rel(u32 index, word<B> offset, u32 type, u32 symbol);

  void add_rela(PackedElfRelA<B, E> &&rel);
  void add_rela(word<B> offset, u32 type, u32 symbol, sword<B> addend);
  void add_rela(word<B> offset, u32 type, std::string_view name, sword<B> addend);
  void replace_rela(u32 index, word<B> offset, u32 type, u32 symbol, sword<B> addend);

  void swap_symbols(u32 first, u32 second) noexcept;

private:
  Shdr &shdr;
  Data &reloc;
  PackedSymbolReader<B, E> symtab;
};
template <ElfBits B, ElfEndian E> using PackedRelocationReader = PackedRelocationAccessor<B, E, true>;
template <ElfBits B, ElfEndian E> using PackedRelocationWriter = PackedRelocationAccessor<B, E, false>;

struct NoteEntry {
  u32 namesz;
  u32 descsz;
  u32 type;
  // Followed by name and desc data
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
  using Data = maybe_const_t<Const, std::vector<u8>>;
  PackedDynamicAccessor(Elf &elf, u16 index);
  PackedDynamicAccessor(Shdr &shdr, Data &data) noexcept;

  u32 entry_count() const noexcept;
  PackedElfDyn<B, E> get_entry(u32 index) const noexcept;
  void add_entry(PackedElfDyn<B, E> &&dyn);
  void add_entry(word<B> tag, word<B> value);
  void add_entry(DynamicTags tag, word<B> value);

private:
  Shdr &shdr;
  Data &data;
};
template <ElfBits B, ElfEndian E> using PackedDynamicReader = PackedDynamicAccessor<B, E, true>;
template <ElfBits B, ElfEndian E> using PackedDynamicWriter = PackedDynamicAccessor<B, E, false>;

/*
 * Used to aceess .init_array, .fini_array, etc. These are effectively arrays of function pointers.
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

// Strings

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

// Symbols

template <ElfBits B, ElfEndian E, bool Const>
PackedSymbolAccessor<B, E, Const>::PackedSymbolAccessor(PackedSymbolAccessor<B, E, Const>::Elf &elf, u16 index)
    : shdr(elf.section_headers[index]), symtab(elf.section_data[index]), strtab(elf, shdr.sh_link) {
  if (index > elf.section_headers.size()) throw std::runtime_error("PackedSymbolWriter: invalid section index");
}

template <ElfBits B, ElfEndian E, bool Const>
PackedSymbolAccessor<B, E, Const>::PackedSymbolAccessor(PackedSymbolAccessor<B, E, Const>::Shdr &shdr_symbol,
                                                        PackedSymbolAccessor<B, E, Const>::Data &symtab,
                                                        PackedSymbolAccessor<B, E, Const>::Shdr &shdr,
                                                        PackedSymbolAccessor<B, E, Const>::Data &strtab) noexcept
    : shdr(shdr_symbol), symtab(symtab), strtab(shdr, strtab) {}

template <ElfBits B, ElfEndian E, bool Const> u32 PackedSymbolAccessor<B, E, Const>::symbol_count() const noexcept {
  if (shdr.sh_entsize == 0 || shdr.sh_size == 0) return 0;
  return shdr.sh_size / shdr.sh_entsize;
}

template <ElfBits B, ElfEndian E, bool Const>
PackedElfSymbol<B, E> PackedSymbolAccessor<B, E, Const>::get_symbol(u32 index) const noexcept {
  auto ptr = get_symbol_ptr(index);
  if (ptr == nullptr) return {};
  return *ptr;
}

template <ElfBits B, ElfEndian E, bool Const>
PackedElfSymbol<B, E> *PackedSymbolAccessor<B, E, Const>::get_symbol_ptr(u32 index) const noexcept {
  if (index >= symbol_count()) return nullptr;
  auto ret = (PackedElfSymbol<B, E> *)(symtab.data() + index * shdr.sh_entsize);
  return ret;
}

template <ElfBits B, ElfEndian E, bool Const>
u32 PackedSymbolAccessor<B, E, Const>::find_symbol_index(std::string_view name) const noexcept {
  // TODO: if there is a hash table index, use it for faster lookup.
  auto str_idx = strtab.find(name);
  if (str_idx == 0 && !name.empty()) return 0;
  for (u32 it = 0; it < symbol_count(); ++it)
    if (auto sym = get_symbol(it); sym.st_name == str_idx) return it;
  return 0;
}

template <ElfBits B, ElfEndian E, bool Const>
std::optional<PackedElfSymbol<B, E>>
PackedSymbolAccessor<B, E, Const>::find_symbol(std::string_view name) const noexcept {
  auto index = find_symbol_index(name);
  if (index == 0 && !name.empty()) return std::nullopt;
  return get_symbol(index);
}

template <ElfBits B, ElfEndian E, bool Const>
std::optional<PackedElfSymbol<B, E>> PackedSymbolAccessor<B, E, Const>::find_symbol(Word<B, E> address) const noexcept {
  for (u32 it = 0; it < symbol_count(); ++it)
    if (auto sym = get_symbol(it); sym.st_value == address) return sym;
  return std::nullopt;
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedSymbolAccessor<B, E, Const>::replace_value(u32 index, Word<B, E> value) noexcept {
  if (index >= symbol_count()) return;
  get_symbol_ptr(index)->st_value = value;
}

template <ElfBits B, ElfEndian E, bool Const>
u32 PackedSymbolAccessor<B, E, Const>::add_symbol(PackedElfSymbol<B, E> &&symbol) {
  if (shdr.sh_size == 0) copy_to_symtab(create_null_symbol<B, E>());
  copy_to_symtab(std::move(symbol));
  return symbol_count() - 1;
}

template <ElfBits B, ElfEndian E, bool Const>
u32 PackedSymbolAccessor<B, E, Const>::add_symbol(PackedElfSymbol<B, E> &&symbol, std::string_view name) {
  if (shdr.sh_size == 0) copy_to_symtab(create_null_symbol<B, E>());
  add_symbol(std::move(symbol), name, symbol.st_shndx);
  return symbol_count() - 1;
}

template <ElfBits B, ElfEndian E, bool Const>
u32 PackedSymbolAccessor<B, E, Const>::add_symbol(PackedElfSymbol<B, E> &&symbol, std::string_view name,
                                                  u16 section_index) {
  if (shdr.sh_size == 0) copy_to_symtab(create_null_symbol<B, E>());
  auto name_idx = strtab.add_string(name);
  symbol.st_name = name_idx;
  symbol.st_shndx = section_index;
  copy_to_symtab(std::move(symbol));
  return symbol_count() - 1;
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedSymbolAccessor<B, E, Const>::arrange_local_symbols(std::function<void(Word<B, E>, Word<B, E>)> func) {
  u32 first_not_local = 1; // Skip the first entry. It is always NOTYPE
  word<B> current = 0, count = symbol_count();

  while (true) {
    PackedElfSymbol<B, E> *p1 = nullptr;
    PackedElfSymbol<B, E> *p2 = nullptr;

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

template <ElfBits B, ElfEndian E, bool Const>
void PackedSymbolAccessor<B, E, Const>::copy_to_symtab(PackedElfSymbol<B, E> &&symbol) {
  if (shdr.sh_entsize == 0) shdr.sh_entsize = sizeof(PackedElfSymbol<B, E>);
  const u8 *ptr = reinterpret_cast<const u8 *>(&symbol);
  symtab.insert(symtab.end(), ptr, ptr + sizeof(PackedElfSymbol<B, E>));
  shdr.sh_size = symtab.size();
}

// Relocations
template <ElfBits B, ElfEndian E, bool Const>
PackedRelocationAccessor<B, E, Const>::PackedRelocationAccessor(PackedRelocationAccessor<B, E, Const>::Elf &elf,
                                                                u16 index)
    : shdr(elf.section_headers[index]), reloc(elf.section_data[index]), symtab(elf, shdr.sh_link) {
  if (index > elf.section_headers.size()) throw std::runtime_error("PackedRelocationWriter: invalid section index");
}

template <ElfBits B, ElfEndian E, bool Const>
PackedRelocationAccessor<B, E, Const>::PackedRelocationAccessor(PackedRelocationAccessor<B, E, Const>::Shdr &shdr_reloc,
                                                                PackedRelocationAccessor<B, E, Const>::Data &reloc,
                                                                const PackedElfShdr<B, E> &shdr_symbol,
                                                                const std::vector<u8> &symtab,
                                                                const PackedElfShdr<B, E> &shdr_strtab,
                                                                std::vector<u8> &strtab) noexcept
    : shdr(shdr_reloc), reloc(reloc), symtab(symtab, shdr_symbol, shdr_strtab, strtab) {}

template <ElfBits B, ElfEndian E, bool Const> bool PackedRelocationAccessor<B, E, Const>::is_rela() const noexcept {
  return (shdr.sh_type == to_underlying(SectionTypes::SHT_RELA));
}

template <ElfBits B, ElfEndian E, bool Const>
u32 PackedRelocationAccessor<B, E, Const>::relocation_count() const noexcept {
  if (shdr.sh_entsize == 0 || shdr.sh_size == 0) return 0;
  return shdr.sh_size / shdr.sh_entsize;
}

template <ElfBits B, ElfEndian E, bool Const>
PackedElfRel<B, E> PackedRelocationAccessor<B, E, Const>::get_rel(u32 index) const noexcept {
  if (index >= relocation_count()) return PackedElfRel<B, E>{};
  return *(PackedElfRel<B, E> *)(reloc.data() + index * shdr.sh_entsize);
}

template <ElfBits B, ElfEndian E, bool Const>
PackedElfRelA<B, E> PackedRelocationAccessor<B, E, Const>::get_rela(u32 index) const noexcept {
  if (index >= relocation_count()) return PackedElfRelA<B, E>{};
  return *(PackedElfRelA<B, E> *)(reloc.data() + index * shdr.sh_entsize);
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedRelocationAccessor<B, E, Const>::add_rel(PackedElfRel<B, E> &&rel) {
  if (shdr.sh_entsize == 0) shdr.sh_entsize = sizeof(PackedElfRel<B, E>);
  const u8 *ptr = reinterpret_cast<const u8 *>(&rel);
  reloc.insert(reloc.end(), ptr, ptr + sizeof(PackedElfRel<B, E>));
  shdr.sh_size = reloc.size();
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedRelocationAccessor<B, E, Const>::add_rel(word<B> offset, u32 type, u32 symbol) {
  PackedElfRel<B, E> rel;
  rel.r_offset = offset;
  if constexpr (B == ElfBits::b32) rel.r_info = pepp::bts::r_info<E>((u8)type, U24<E>{symbol});
  else rel.r_info = pepp::bts::r_info<E>(U32<E>{type}, U32<E>{symbol});
  add_rel(std::move(rel));
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedRelocationAccessor<B, E, Const>::add_rel(word<B> offset, u32 type, std::string_view name) {
  auto symbol = symtab.find_symbol_index(name);
  if (symbol == 0) throw std::runtime_error("PackedRelocationWriter: symbol not found: " + std::string(name));
  PackedElfRel<B, E> rel;
  rel.r_offset = offset;
  if constexpr (B == ElfBits::b32) rel.r_info = pepp::bts::r_info<E>((u8)type, U24<E>{symbol});
  else rel.r_info = pepp::bts::r_info<E>(U32<E>{type}, U32<E>{symbol});

  add_rel(std::move(rel));
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedRelocationAccessor<B, E, Const>::replace_rel(u32 index, word<B> offset, u32 type, u32 symbol) {
  if (index >= relocation_count()) throw std::runtime_error("PackedRelocationWriter: invalid relocation index");
  PackedElfRel<B, E> *rel = reinterpret_cast<PackedElfRel<B, E> *>(reloc.data() + index * shdr.sh_entsize);
  rel->r_offset = offset;
  if constexpr (B == ElfBits::b32) rel->r_info = pepp::bts::r_info<E>((u8)type, U24<E>{symbol});
  else rel->r_info = pepp::bts::r_info<E>(U32<E>{type}, U32<E>{symbol});
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedRelocationAccessor<B, E, Const>::add_rela(PackedElfRelA<B, E> &&rel) {
  if (shdr.sh_entsize == 0) shdr.sh_entsize = sizeof(PackedElfRelA<B, E>);
  const u8 *ptr = reinterpret_cast<const u8 *>(&rel);
  reloc.insert(reloc.end(), ptr, ptr + sizeof(PackedElfRelA<B, E>));
  shdr.sh_size = reloc.size();
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedRelocationAccessor<B, E, Const>::add_rela(word<B> offset, u32 type, u32 symbol, sword<B> addend) {
  PackedElfRelA<B, E> rela;
  rela.r_offset = offset;
  if constexpr (B == ElfBits::b32) rela.r_info = pepp::bts::r_info<E>((u8)type, U24<E>{symbol});
  else rela.r_info = pepp::bts::r_info<E>(U32<E>{type}, U32<E>{symbol});
  rela.r_addend = addend;
  add_rela(std::move(rela));
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedRelocationAccessor<B, E, Const>::add_rela(word<B> offset, u32 type, std::string_view name, sword<B> addend) {
  auto symbol = symtab.find_symbol_index(name);
  if (symbol == 0) throw std::runtime_error("PackedRelocationWriter: symbol not found: " + std::string(name));
  PackedElfRelA<B, E> rela;
  rela.r_offset = offset;
  if constexpr (B == ElfBits::b32) rela.r_info = pepp::bts::r_info<E>((u8)type, U24<E>{symbol});
  else rela.r_info = pepp::bts::r_info<E>(U32<E>{type}, U32<E>{symbol});
  rela.r_addend = addend;
  add_rela(std::move(rela));
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedRelocationAccessor<B, E, Const>::replace_rela(u32 index, word<B> offset, u32 type, u32 symbol,
                                                         sword<B> addend) {
  if (index >= relocation_count()) throw std::runtime_error("PackedRelocationWriter: invalid relocation index");
  PackedElfRelA<B, E> *rela = reinterpret_cast<PackedElfRelA<B, E> *>(reloc.data() + index * shdr.sh_entsize);
  rela->r_offset = offset;
  if constexpr (B == ElfBits::b32) rela->r_info = pepp::bts::r_info<E>((u8)type, U24<E>{symbol});
  else rela->r_info = pepp::bts::r_info<E>(U32<E>{type}, U32<E>{symbol});
  rela->r_addend = addend;
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedRelocationAccessor<B, E, Const>::swap_symbols(u32 first, u32 second) noexcept {
  // Helpers to do the 32- vs 64-bit packing for REL/RELA type|symbol
  static const auto update_rel = [](PackedElfRel<B, E> *rela, u32 new_sym) {
    if constexpr (B == ElfBits::b32) {
      auto type = pepp::bts::r_type<E>(U32<E>{rela->r_info});
      rela->r_info = pepp::bts::r_info<E>((u8)type, U24<E>{new_sym});
    } else {
      auto type = pepp::bts::r_type<E>(U64<E>{rela->r_info});
      rela->r_info = pepp::bts::r_info<E>(U32<E>{type}, U32<E>{new_sym});
    }
  };
  static const auto update_rela = [](PackedElfRelA<B, E> *rela, u32 new_sym) {
    if constexpr (B == ElfBits::b32) {
      auto type = pepp::bts::r_type<E>(U32<E>{rela->r_info});
      rela->r_info = pepp::bts::r_info<E>((u8)type, U24<E>{new_sym});
    } else {
      auto type = pepp::bts::r_type<E>(U64<E>{rela->r_info});
      rela->r_info = pepp::bts::r_info<E>(U32<E>{type}, U32<E>{new_sym});
    }
  };

  for (int it = 0; it < relocation_count(); it++) {
    if (is_rela()) {
      PackedElfRelA<B, E> *rela = reinterpret_cast<PackedElfRelA<B, E> *>(reloc.data() + it * shdr.sh_entsize);
      u32 sym = B == ElfBits::b64 ? r_sym<E>(U64<E>{rela->r_info}) : r_sym<E>(U32<E>{rela->r_info});
      if (sym == first) update_rela(rela, second);
      else if (sym == second) update_rela(rela, first);
    } else {
      PackedElfRel<B, E> *rel = reinterpret_cast<PackedElfRel<B, E> *>(reloc.data() + it * shdr.sh_entsize);
      u32 sym = B == ElfBits::b64 ? r_sym<E>(U64<E>{rel->r_info}) : r_sym<E>(U32<E>{rel->r_info});
      if (sym == first) update_rel(rel, second);
      else if (sym == second) update_rel(rel, first);
    }
  }
}

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
  if (index * shdr.sh_entsize + sizeof(PackedElfDyn<B, E>) > data.size()) return PackedElfDyn<B, E>{};
  return *(PackedElfDyn<B, E> *)(data.data() + index * shdr.sh_entsize);
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedDynamicAccessor<B, E, Const>::add_entry(PackedElfDyn<B, E> &&dyn) {
  if (shdr.sh_entsize == 0) shdr.sh_entsize = sizeof(PackedElfDyn<B, E>);
  data.insert(data.end(), reinterpret_cast<const u8 *>(&dyn),
              reinterpret_cast<const u8 *>(&dyn) + sizeof(PackedElfDyn<B, E>));
  shdr.sh_size = data.size();
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedDynamicAccessor<B, E, Const>::add_entry(word<B> tag, word<B> value) {
  PackedElfDyn<B, E> dyn;
  dyn.d_tag = tag;
  dyn.d_val = value;
  add_entry(std::move(dyn));
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedDynamicAccessor<B, E, Const>::add_entry(DynamicTags tag, word<B> value) {
  add_entry(to_underlying(tag), value);
}

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
  const u8 *ptr = reinterpret_cast<const u8 *>(&adjust);
  data.insert(data.end(), ptr, ptr + sizeof(Word<B, E>));
  shdr.sh_size = data.size();
}
} // namespace pepp::bts
