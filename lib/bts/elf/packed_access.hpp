#pragma once
#include "./packed_elf.hpp"
#include "bts/bitmanip/log2.hpp"
#include "bts/elf/packed_fixup.hpp"
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
  bits::span<const char> get_symbol_name(u32 index) const noexcept;
  u32 find_symbol(std::string_view name) const noexcept;
  u32 find_symbol(Word<B, E> value) const noexcept;

  // Add a symbol to the table. Assumes you already set st_name!
  u32 add_symbol(PackedElfSymbol<B, E> &&symbol);
  // Helpers to assign name and section index.
  u32 add_symbol(PackedElfSymbol<B, E> &&symbol, std::string_view name);
  u32 add_symbol(PackedElfSymbol<B, E> &&symbol, std::string_view name, u16 section_index);
  void replace_symbol(u32 index, PackedElfSymbol<B, E> &&symbol) noexcept;
  void replace_value(u32 index, Word<B, E> value) noexcept;
  // DANGER! The index will not be updated by arrange_local_symbols.
  AbsoluteFixup fixup_value(word<B> index, std::function<word<B>()> f);
  void arrange_local_symbols(std::function<void(Word<B, E> first, Word<B, E> second)> func = nullptr);

protected:
  void copy_to_symtab(PackedElfSymbol<B, E> &&symbol);
  void swap_symbols(u32 first, u32 second) noexcept;
  Shdr &shdr_symtab;
  Data &data_symtab;
  PackedStringAccessor<B, E, Const> strtab;
};
template <ElfBits B, ElfEndian E> using PackedSymbolReader = PackedSymbolAccessor<B, E, true>;
template <ElfBits B, ElfEndian E> using PackedSymbolWriter = PackedSymbolAccessor<B, E, false>;

/*
 * Per:
 *   https://flapenguin.me/elf-dt-hash
 *   https://refspecs.linuxfoundation.org/elf/gabi4+/ch5.dynamic.html#hash
 *
 * Hash table has 2 u32 headers: nbuckets, symndx, maskwords, shift.
 * layout is roughly:
 * template <ElfBits B> struct {
 *   u32 nbuckets, nchains;
 *   u32 buckets[nbuckets];
 *   u32 chains[nchains];
 * };
 * buckets[n] points to the head of a chain or 0.
 * Index into it via hash(symbol_name)%nbuckets.
 * buckets[n] contains index into the symbol table which also indexes chains.
 *
 * chains[n] points to the next element in a linked list of symbol indices or 0
 * Keep crawling the chain until you find the target symbol or you reach 0.
 *
 * For outside readers to take advantage of this section, you must do extra work beyond creating the hash table:
 * - .dynsym, .dynstr, and .dynamic must:
 *   - be part of a contiguous PT_DYNAMIC segment
 *   - be part of a contiguous PT_LOAD segment
 *   - have a correctly computed sh_addr
 * - _DYNAMIC symbol must exist and point to the vaddr first entry of .dynamic
 * - PT_LOAD and PT_DYNAMIC segments must have p_vaddr and p_paddr set correctly, and a page-aligned memory size
 * - DYNAMIC array entries
 *   - DT_STRTAB, pointing to VADDR of .dynstr
 *   - DT_STRSZ, size of .dynstr
 *   - DT_SYMTAB, pointing to VADDR of .dynsym
 *   - DT_HASH, pointing to VADDR of .hash
 */
u32 elf_hash(bits::span<const char>) noexcept;
u32 elf_hash(std::string_view) noexcept;
template <ElfBits B, ElfEndian E, bool Const>
class PackedHashedSymbolAccessor : public PackedSymbolAccessor<B, E, Const> {
public:
  using Elf = maybe_const_t<Const, PackedElf<B, E>>;
  using Shdr = maybe_const_t<Const, PackedElfShdr<B, E>>;
  using Data = maybe_const_t<Const, std::vector<u8>>;
  PackedHashedSymbolAccessor(Elf &elf, u16 index);
  PackedHashedSymbolAccessor(Shdr &shdr_hash, Data &data_hash, Shdr &shdr_symbol, Data &data_symbol, Shdr &shdr_strtab,
                             Data &data_strtab) noexcept;
  // return index into underlying symbol table, or 0 if not found
  // will not find unhashed symbols; use base class's find_symbol for exhaustive search.
  u32 find_hashed_symbol(std::string_view name) const noexcept;
  void compute_hash_table(u32 nbuckets);

  u32 nbuckets() const noexcept;
  u32 nchains() const noexcept;
  bits::span<const U32<E>> buckets() const noexcept;
  bits::span<const U32<E>> chains() const noexcept;

private:
  Shdr &shdr_hash;
  Data &data_hash;
};
template <ElfBits B, ElfEndian E> using PackedHashedSymbolReader = PackedHashedSymbolAccessor<B, E, true>;
template <ElfBits B, ElfEndian E> using PackedHashedSymbolWriter = PackedHashedSymbolAccessor<B, E, false>;

/*
 * Per:
 *   https://blogs.oracle.com/solaris/gnu-hash-elf-sections-v2
 *   https://flapenguin.me/elf-dt-gnu-hash
 *   https://sourceware.org/pipermail/binutils/2006-October/049450.html
 *
 * Hash table has 4 u32 headers: nbuckets, symndx, maskwords, shift.
 * layout is roughly:
 * template <ElfBits B> struct {
 *   u32 nbuckets, symndx, maskwords, shift2;
 *   word<B> bloom[maskwords];
 *   u32 buckets[nbuckets];
 *   u32 chains[# of symbols hashed];
 * };
 * Bloom filter is used to quickly rule out misses using 2 functions (bits) per symbol
 *
 * For outside readers to take advantage of this section, you must do extra work beyond creating the hash table:
 * - .dynsym, .dynstr, and .dynamic must:
 *   - be part of a contiguous PT_DYNAMIC segment
 *   - be part of a contiguous PT_LOAD segment
 *   - have a correctly computed sh_addr
 * - _DYNAMIC symbol must exist and point to the vaddr first entry of .dynamic
 * - PT_LOAD and PT_DYNAMIC segments must have p_vaddr and p_paddr set correctly, and a page-aligned memory size
 * - DYNAMIC array entries
 *   - DT_STRTAB, pointing to VADDR of .dynstr
 *   - DT_STRSZ, size of .dynstr
 *   - DT_SYMTAB, pointing to VADDR of .dynsym
 *   - DT_GNU_HASH, pointing to VADDR of .gnu.hash
 */
u32 gnu_elf_hash(bits::span<const char>) noexcept;
u32 gnu_elf_hash(std::string_view) noexcept;
template <ElfBits B, ElfEndian E, bool Const>
class PackedGNUHashedSymbolAccessor : public PackedSymbolAccessor<B, E, Const> {
public:
  using Elf = maybe_const_t<Const, PackedElf<B, E>>;
  using Shdr = maybe_const_t<Const, PackedElfShdr<B, E>>;
  using Data = maybe_const_t<Const, std::vector<u8>>;
  PackedGNUHashedSymbolAccessor(Elf &elf, u16 index);
  PackedGNUHashedSymbolAccessor(Shdr &shdr_hash, Data &data_hash, Shdr &shdr_symbol, Data &data_symbol,
                                Shdr &shdr_strtab, Data &data_strtab) noexcept;
  // return index into underlying symbol table, or 0 if not found
  // will not find unhashed symbols; use base class's find_symbol for exhaustive search.
  u32 find_hashed_symbol(std::string_view name) const noexcept;
  void compute_hash_table(u32 nbuckets, u32 symndx, u32 maskwords, u32 shift2,
                          std::function<void(Word<B, E>, Word<B, E>)> func = nullptr);

  u32 nbuckets() const noexcept;
  u32 symndx() const noexcept;
  u32 maskwords() const noexcept;
  u32 mshift2() const noexcept;
  bits::span<const Word<B, E>> bloom() const noexcept;
  bits::span<const U32<E>> buckets() const noexcept;
  bits::span<const U32<E>> chains() const noexcept;

private:
  Shdr &shdr_hash;
  Data &data_hash;
};
template <ElfBits B, ElfEndian E> using PackedGNUHashedSymbolReader = PackedGNUHashedSymbolAccessor<B, E, true>;
template <ElfBits B, ElfEndian E> using PackedGNUHashedSymbolWriter = PackedGNUHashedSymbolAccessor<B, E, false>;

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
  PackedElfDyn<B, E> *get_entry_ptr(u32 index) const noexcept;
  u32 add_entry(PackedElfDyn<B, E> &&dyn);
  u32 add_entry(word<B> tag); // Add entry with 0 value. Often used for placeholders
  u32 add_entry(word<B> tag, word<B> value);
  u32 add_entry(DynamicTags tag); // Add entry with 0 value. Often used for placeholders
  u32 add_entry(DynamicTags tag, word<B> value);
  void replace_entry(u32 index, PackedElfDyn<B, E> &&dyn) noexcept;
  void replace_entry(u32 index, word<B> tag, word<B> value) noexcept;
  AbsoluteFixup fixup_value(u32 index, std::function<word<B>()> f);

private:
  Shdr &shdr;
  Data &data;
};
template <ElfBits B, ElfEndian E> using PackedDynamicReader = PackedDynamicAccessor<B, E, true>;
template <ElfBits B, ElfEndian E> using PackedDynamicWriter = PackedDynamicAccessor<B, E, false>;

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

// Symbols

template <ElfBits B, ElfEndian E, bool Const>
PackedSymbolAccessor<B, E, Const>::PackedSymbolAccessor(PackedSymbolAccessor<B, E, Const>::Elf &elf, u16 index)
    : shdr_symtab(elf.section_headers[index]), data_symtab(elf.section_data[index]), strtab(elf, shdr_symtab.sh_link) {
  if (index > elf.section_headers.size()) throw std::runtime_error("PackedSymbolWriter: invalid section index");
}

template <ElfBits B, ElfEndian E, bool Const>
PackedSymbolAccessor<B, E, Const>::PackedSymbolAccessor(PackedSymbolAccessor<B, E, Const>::Shdr &shdr_symbol,
                                                        PackedSymbolAccessor<B, E, Const>::Data &symtab,
                                                        PackedSymbolAccessor<B, E, Const>::Shdr &shdr,
                                                        PackedSymbolAccessor<B, E, Const>::Data &strtab) noexcept
    : shdr_symtab(shdr_symbol), data_symtab(symtab), strtab(shdr, strtab) {}

template <ElfBits B, ElfEndian E, bool Const> u32 PackedSymbolAccessor<B, E, Const>::symbol_count() const noexcept {
  if (shdr_symtab.sh_entsize == 0 || shdr_symtab.sh_size == 0) return 0;
  return shdr_symtab.sh_size / shdr_symtab.sh_entsize;
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
  auto ret = (PackedElfSymbol<B, E> *)(data_symtab.data() + index * shdr_symtab.sh_entsize);
  return ret;
}

template <ElfBits B, ElfEndian E, bool Const>
bits::span<const char> PackedSymbolAccessor<B, E, Const>::get_symbol_name(u32 index) const noexcept {
  if (index >= symbol_count()) return {};
  return strtab.get_string_span(get_symbol(index).st_name);
}

template <ElfBits B, ElfEndian E, bool Const>
u32 PackedSymbolAccessor<B, E, Const>::find_symbol(std::string_view name) const noexcept {
  auto str_idx = strtab.find(name);
  if (str_idx == 0 && !name.empty()) return 0;
  for (u32 it = 0; it < symbol_count(); ++it)
    if (auto sym = get_symbol(it); sym.st_name == str_idx) return it;
  return 0;
}

template <ElfBits B, ElfEndian E, bool Const>
u32 PackedSymbolAccessor<B, E, Const>::find_symbol(Word<B, E> address) const noexcept {
  for (u32 it = 0; it < symbol_count(); ++it)
    if (auto sym = get_symbol(it); sym.st_value == address) return it;
  return 0;
}

template <ElfBits B, ElfEndian E, bool Const>
u32 PackedSymbolAccessor<B, E, Const>::add_symbol(PackedElfSymbol<B, E> &&symbol) {
  if (shdr_symtab.sh_size == 0) copy_to_symtab(create_null_symbol<B, E>());
  copy_to_symtab(std::move(symbol));
  return symbol_count() - 1;
}

template <ElfBits B, ElfEndian E, bool Const>
u32 PackedSymbolAccessor<B, E, Const>::add_symbol(PackedElfSymbol<B, E> &&symbol, std::string_view name) {
  auto idx = symbol.st_shndx;
  add_symbol(std::move(symbol), name, idx);
  return symbol_count() - 1;
}

template <ElfBits B, ElfEndian E, bool Const>
u32 PackedSymbolAccessor<B, E, Const>::add_symbol(PackedElfSymbol<B, E> &&symbol, std::string_view name,
                                                  u16 section_index) {
  if (shdr_symtab.sh_size == 0) copy_to_symtab(create_null_symbol<B, E>());
  auto name_idx = strtab.add_string(name);
  symbol.st_name = name_idx;
  symbol.st_shndx = section_index;
  copy_to_symtab(std::move(symbol));
  return symbol_count() - 1;
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedSymbolAccessor<B, E, Const>::replace_symbol(u32 index, PackedElfSymbol<B, E> &&symbol) noexcept {
  if (index >= symbol_count()) return;
  *get_symbol_ptr(index) = symbol;
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedSymbolAccessor<B, E, Const>::replace_value(u32 index, Word<B, E> value) noexcept {
  if (index >= symbol_count()) return;
  get_symbol_ptr(index)->st_value = value;
}

template <ElfBits B, ElfEndian E, bool Const>
AbsoluteFixup PackedSymbolAccessor<B, E, Const>::fixup_value(word<B> index, std::function<word<B>()> func) {
  return AbsoluteFixup{.update = [this, index, func]() {
    if (index >= symbol_count()) return;
    PackedElfSymbol<B, E> *sym = get_symbol_ptr(index);
    sym->st_value = func();
  }};
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
      swap_symbols(first_not_local, current);
    } else {
      // Update 'info' field of the section
      shdr_symtab.sh_info = first_not_local;
      break;
    }
  }
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedSymbolAccessor<B, E, Const>::copy_to_symtab(PackedElfSymbol<B, E> &&symbol) {
  if (shdr_symtab.sh_entsize == 0) shdr_symtab.sh_entsize = sizeof(PackedElfSymbol<B, E>);
  const u8 *ptr = reinterpret_cast<const u8 *>(&symbol);
  data_symtab.insert(data_symtab.end(), ptr, ptr + sizeof(PackedElfSymbol<B, E>));
  shdr_symtab.sh_size = data_symtab.size();
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedSymbolAccessor<B, E, Const>::swap_symbols(u32 first, u32 second) noexcept {
  const auto cnt = symbol_count();
  // Don't allow swapping the null symbol
  if (first >= cnt || second >= cnt || first == 0 || second == 0) return;
  auto ptr1 = get_symbol_ptr(first), ptr2 = get_symbol_ptr(second);
  std::swap(*ptr1, *ptr2);
}

template <ElfBits B, ElfEndian E, bool Const>
PackedHashedSymbolAccessor<B, E, Const>::PackedHashedSymbolAccessor(Elf &elf, u16 index)
    : pepp::bts::PackedSymbolAccessor<B, E, Const>(elf, elf.section_headers[index].sh_link),
      shdr_hash(elf.section_headers[index]), data_hash(elf.section_data[index]) {}

template <ElfBits B, ElfEndian E, bool Const>
PackedHashedSymbolAccessor<B, E, Const>::PackedHashedSymbolAccessor(Shdr &shdr_hash, Data &data_hash, Shdr &shdr_symbol,
                                                                    Data &data_symbol, Shdr &shdr_strtab,
                                                                    Data &data_strtab) noexcept
    : PackedSymbolAccessor<B, E, Const>(shdr_symbol, data_symbol, shdr_strtab, data_strtab), shdr_hash(shdr_hash),
      data_hash(data_hash) {}

template <ElfBits B, ElfEndian E, bool Const>
u32 PackedHashedSymbolAccessor<B, E, Const>::find_hashed_symbol(std::string_view name) const noexcept {
  const auto hash = elf_hash(name);
  const auto nbucket = this->nbuckets(), nchain = this->nchains();
  if (nbucket == 0 || nchain == 0) return 0;
  const auto bucket = this->buckets(), chain = this->chains();
  for (u32 i = bucket[hash % nbucket]; i; i = chain[i]) {
    auto lhs = this->get_symbol_name(i);
    if (lhs.size() == name.size() && std::equal(lhs.begin(), lhs.end(), name.begin())) return i;
  }
  return 0;
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedHashedSymbolAccessor<B, E, Const>::compute_hash_table(u32 nbuckets) {
  const auto hashed_count = this->symbol_count();
  // Heuristic: About 2 symbols per bucket (avoid 0).
  if (nbuckets == 0) nbuckets = std::max<u32>(1, hashed_count / 2);

  // 0 indicates that there is no symbol with that hash / stop crawling the linked list.
  std::vector<u32> buckets(nbuckets, 0), chains(hashed_count, 0);
  // System-V does not care about the order of chain items so I am free to order the linked lists however I want.
  // I've chosen the following invariant to be true:  for all i chains[i] == 0 || chains[i] > i
  // This has the effect of creating linked lists which grow from low address in the table to high address.
  for (i32 i = hashed_count; i > 0; i--) {
    u32 b = elf_hash(this->get_symbol_name(i)) % nbuckets;
    chains[i] = buckets[b]; // next is previous head (0 if empty)
    buckets[b] = i;         // new head is this symbol
  }

  // Write out the hash table in correct endian format. Avoid reallocations of underlying buffer.
  data_hash.clear();
  this->data_hash.reserve(2 * sizeof(u32) + +nbuckets * sizeof(u32) + chains.size() * sizeof(u32));

  // Helper to conditionally byteswap and append a u32
  auto append_u32 = [](auto &data_hash, u32 val) {
    u8 buf[4];
    *((U32<E> *)buf) = val;
    data_hash.insert(data_hash.end(), buf, buf + 4);
  };

  // Header, buckets, chains
  append_u32(data_hash, nbuckets);
  append_u32(data_hash, chains.size());
  for (const auto &b : buckets) append_u32(data_hash, b);
  for (const auto &c : chains) append_u32(data_hash, c);
}

template <ElfBits B, ElfEndian E, bool Const> u32 PackedHashedSymbolAccessor<B, E, Const>::nbuckets() const noexcept {
  if (data_hash.size() < 2 * sizeof(u32)) return 0;
  return u32{*((U32<E> *)data_hash.data() + 0)};
}

template <ElfBits B, ElfEndian E, bool Const> u32 PackedHashedSymbolAccessor<B, E, Const>::nchains() const noexcept {
  if (data_hash.size() < 2 * sizeof(u32)) return 0;
  return u32{*((U32<E> *)data_hash.data() + 1)};
}

template <ElfBits B, ElfEndian E, bool Const>
bits::span<const U32<E>> PackedHashedSymbolAccessor<B, E, Const>::buckets() const noexcept {
  if (data_hash.size() < 4 * sizeof(u32)) return {};
  return bits::span<const U32<E>>((const U32<E> *)(data_hash.data() + 2 * sizeof(u32)), nbuckets());
}

template <ElfBits B, ElfEndian E, bool Const>
bits::span<const U32<E>> PackedHashedSymbolAccessor<B, E, Const>::chains() const noexcept {
  const auto offset = 2 * sizeof(uint32_t) + nbuckets() * sizeof(U32<E>);
  if (data_hash.size() < offset) return {};
  return bits::span<const U32<E>>{(const U32<E> *)(data_hash.data() + offset), nchains()};
}

template <ElfBits B, ElfEndian E, bool Const>
PackedGNUHashedSymbolAccessor<B, E, Const>::PackedGNUHashedSymbolAccessor(Elf &elf, u16 index)
    : pepp::bts::PackedSymbolAccessor<B, E, Const>(elf, elf.section_headers[index].sh_link),
      shdr_hash(elf.section_headers[index]), data_hash(elf.section_data[index]) {}

template <ElfBits B, ElfEndian E, bool Const>
PackedGNUHashedSymbolAccessor<B, E, Const>::PackedGNUHashedSymbolAccessor(Shdr &shdr_hash, Data &data_hash,
                                                                          Shdr &shdr_symbol, Data &data_symbol,
                                                                          Shdr &shdr_strtab, Data &data_strtab) noexcept
    : PackedSymbolAccessor<B, E, Const>(shdr_symbol, data_symbol, shdr_strtab, data_strtab), shdr_hash(shdr_hash),
      data_hash(data_hash) {}

template <ElfBits B, ElfEndian E, bool Const>
u32 PackedGNUHashedSymbolAccessor<B, E, Const>::find_hashed_symbol(std::string_view name) const noexcept {
  using ElfSymbol = PackedElfSymbol<B, E>;

  const u32 bloom_size = this->maskwords();
  const u32 bloom_shift = this->mshift2();
  const auto nbuckets = this->nbuckets();
  const auto bloom_filter = this->bloom();
  u32 hash = gnu_elf_hash(name);
  u32 bloom_index = (hash / (8 * sizeof(ElfSymbol))) % bloom_size;
  Word<B, E> bloom_bits = ((Word<B, E>)1 << (hash % (8 * sizeof(Word<B, E>)))) |
                          ((Word<B, E>)1 << ((hash >> bloom_shift) % (8 * sizeof(Word<B, E>))));

  if (Word<B, E>{bloom_filter[bloom_index]} & bloom_bits != bloom_bits) return 0;

  u32 bucket = hash % nbuckets;
  const u32 symoffset = this->symndx();
  const auto buckets = this->buckets();
  const auto chains = this->chains();

  if (buckets[bucket] >= symoffset) {
    u32 chain_index = buckets[bucket] - symoffset, chain_hash = chains[chain_index];
    std::string symname;

    while (true) {
      if ((chain_hash >> 1) == (hash >> 1)) {
        auto lhs = this->get_symbol_name(symoffset + chain_index);
        if (lhs.size() == name.size() && std::equal(lhs.begin(), lhs.end(), name.begin()))
          return symoffset + chain_index;
      }
      if (chain_hash & 1) break;
      chain_hash = chains[++chain_index];
    }
  }
  return 0;
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedGNUHashedSymbolAccessor<B, E, Const>::compute_hash_table(u32 nbuckets, u32 symndx, u32 maskwords, u32 shift2,
                                                                    std::function<void(Word<B, E>, Word<B, E>)> func) {
  const u32 nsyms = this->symbol_count();
  if (symndx > nsyms) symndx = nsyms;
  const u32 hashed_count = nsyms - symndx;

  // Heuristic: About 2 symbols per bucket (avoid 0).
  if (nbuckets == 0) nbuckets = std::max<u32>(1, hashed_count / 2);
  // Heuristic: bloom words scale with hashed symbols; must be power of two.
  if (maskwords == 0) maskwords = 1 << bits::ceil_log2(std::max<u32>(1, hashed_count / 8));
  else maskwords = 1 << bits::ceil_log2(maskwords);
  if (shift2 == 0) shift2 = 5; // Common linker choice; 5 is widely used.

  // Pre-compute hashes for all input strings
  std::vector<u32> H(hashed_count);
  for (u32 i = 0; i < hashed_count; ++i) H[i] = gnu_elf_hash(this->get_symbol_name(symndx + i));

  // Order the hashes by the bucket into which the fall in. i.e., reorder symbols by ascending hash % nbuckets.
  // Compute the desired order ahead-of-time before applying it. After the sort, for all i, perm[i] is the new target
  // position for the symbol current at i. We must keep unhashed prefix [0, symoffset) intact.
  std::vector<u32> perm(hashed_count);
  std::iota(perm.begin(), perm.end(), 0);
  std::stable_sort(perm.begin(), perm.end(), [&](u32 a, u32 b) { return (H[a] % nbuckets) < (H[b] % nbuckets); });

  // Apply permuatation by swapping in-place using a cycle walk. perm[new] = old  =>  dest[old] = new
  // This avoids allocations of temporary symbols and made it easier to notify other sections of symbol swaps.
  std::vector<std::uint32_t> dest(hashed_count);
  for (std::uint32_t newpos = 0; newpos < hashed_count; ++newpos) dest[perm[newpos]] = newpos;
  for (std::uint32_t i = 0; i < hashed_count; ++i) {
    while (dest[i] != i) {
      u32 j = dest[i];                            // element at i should go to j
      if (func) func(symndx + i, symndx + j);     // Notify other sections of symbol swap
      this->swap_symbols(symndx + i, symndx + j); // Swap symbol entries
      std::swap(H[i], H[j]);                      // Swap hashes
      std::swap(dest[i], dest[j]);                // Keep mapping consistent after swap
    }
  }

  // Since maskwords is power-of-to, we can replace all % maskwords with & (maskwords -1)
  static const u32 maskwords_bitmask = maskwords - 1;
  // Compute bloom filter, per: https://blogs.oracle.com/solaris/gnu-hash-elf-sections-v2
  // Set 2 bits per symbol at the same array index. The array index is a function of the hash.
  // If either bit is 0, the symbol is definitely not present. When both are 1, we need to resort to crawling the chain.
  std::vector<word<B>> bloom_filter(maskwords, 0);
  constexpr std::uint32_t WordBits = 8u * sizeof(word<B>);
  for (u32 i = 0; i < hashed_count; ++i) {
    const u32 H1 = H[i], H2 = (H1 >> shift2);
    const u32 N = ((H1 / WordBits) & maskwords_bitmask);
    const u32 bitmask = (u32{1} << (H1 % WordBits)) | (u32{1} << (H2 % WordBits));
    bloom_filter[N] |= bitmask;
  }

  // Store lowest symbol index for each bucket; 0 means no symbols are in that bucket.
  std::vector<u32> buckets(nbuckets, 0);
  for (u32 i = 0; i < hashed_count; ++i)
    if (const u32 b = H[i] % nbuckets; buckets[b] == 0) buckets[b] = symndx + i;

  // Since we reordered the symbols by bucket, all symbols for a given bucket are contiguous.
  // The last symbol in each bucket is marked by setting low bit in the chain array.
  std::vector<u32> chains(hashed_count, 0);
  for (std::uint32_t i = 0; i < hashed_count; ++i) {
    const u32 h = H[i] & ~1u, this_bucket = H[i] % nbuckets;
    const bool end = (i + 1 == hashed_count) || ((H[i + 1] % nbuckets) != this_bucket);
    chains[i] = h | (end ? 1u : 0u);
  }

  // Write out the hash table in correct endian format. Avoid reallocations of underlying buffer.
  data_hash.clear();
  this->data_hash.reserve(4 * sizeof(u32) + maskwords * sizeof(word<B>) + nbuckets * sizeof(u32) +
                          chains.size() * sizeof(u32));

  // Helper to conditionally byteswap and append a u32
  auto append_u32 = [](auto &data_hash, u32 val) {
    u8 buf[4];
    *((U32<E> *)buf) = val;
    data_hash.insert(data_hash.end(), buf, buf + 4);
  };

  // Header
  append_u32(data_hash, nbuckets);
  append_u32(data_hash, symndx);
  append_u32(data_hash, maskwords);
  append_u32(data_hash, shift2);
  // Bloom filter, buckets, chains
  for (const auto &b : bloom_filter) {
    u8 buf[sizeof(word<B>)];
    *((Word<B, E> *)buf) = b;
    data_hash.insert(data_hash.end(), buf, buf + sizeof(word<B>));
  }
  for (const auto &b : buckets) append_u32(data_hash, b);
  for (const auto &c : chains) append_u32(data_hash, c);
}

template <ElfBits B, ElfEndian E, bool Const>
inline u32 PackedGNUHashedSymbolAccessor<B, E, Const>::nbuckets() const noexcept {
  if (data_hash.size() < 4 * sizeof(u32)) return 0;
  return u32{*((U32<E> *)data_hash.data() + 0)};
}

template <ElfBits B, ElfEndian E, bool Const>
inline u32 PackedGNUHashedSymbolAccessor<B, E, Const>::symndx() const noexcept {
  if (data_hash.size() < 4 * sizeof(u32)) return 0;
  return {*((U32<E> *)data_hash.data() + 1)};
}

template <ElfBits B, ElfEndian E, bool Const>
inline u32 PackedGNUHashedSymbolAccessor<B, E, Const>::maskwords() const noexcept {
  if (data_hash.size() < 4 * sizeof(u32)) return 0;
  return u32{*((U32<E> *)data_hash.data() + 2)};
}

template <ElfBits B, ElfEndian E, bool Const>
inline u32 PackedGNUHashedSymbolAccessor<B, E, Const>::mshift2() const noexcept {
  if (data_hash.size() < 4 * sizeof(u32)) return 0;
  return u32{*((U32<E> *)data_hash.data() + 3)};
}

template <ElfBits B, ElfEndian E, bool Const>
inline bits::span<const Word<B, E>> PackedGNUHashedSymbolAccessor<B, E, Const>::bloom() const noexcept {
  if (data_hash.size() < 4 * sizeof(u32)) return {};
  return bits::span<const Word<B, E>>((const Word<B, E> *)(data_hash.data() + 4 * sizeof(u32)), maskwords());
}

template <ElfBits B, ElfEndian E, bool Const>
inline bits::span<const U32<E>> PackedGNUHashedSymbolAccessor<B, E, Const>::buckets() const noexcept {
  const auto offset = 4 * sizeof(uint32_t) + maskwords() * sizeof(Word<B, E>);
  if (data_hash.size() < offset) return {};
  return bits::span<const U32<E>>{(const U32<E> *)(data_hash.data() + offset), nbuckets()};
}

template <ElfBits B, ElfEndian E, bool Const>
inline bits::span<const U32<E>> PackedGNUHashedSymbolAccessor<B, E, Const>::chains() const noexcept {
  const auto offset = 4 * sizeof(uint32_t) + maskwords() * sizeof(Word<B, E>) + nbuckets() * sizeof(u32);
  const auto end = data_hash.size(), size = (end - offset) / sizeof(u32);
  if (data_hash.size() < offset || size == 0) return {};
  return bits::span<const U32<E>>{(const U32<E> *)(data_hash.data() + offset), size};
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
  auto symbol = symtab.find_symbol(name);
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
  if (auto ptr = get_entry_ptr(index); ptr != nullptr) return *ptr;
  return PackedElfDyn<B, E>{};
}

template <ElfBits B, ElfEndian E, bool Const>
PackedElfDyn<B, E> *PackedDynamicAccessor<B, E, Const>::get_entry_ptr(u32 index) const noexcept {
  if (index * shdr.sh_entsize + sizeof(PackedElfDyn<B, E>) > data.size()) return nullptr;
  return (PackedElfDyn<B, E> *)(data.data() + index * shdr.sh_entsize);
}

template <ElfBits B, ElfEndian E, bool Const>
u32 PackedDynamicAccessor<B, E, Const>::add_entry(PackedElfDyn<B, E> &&dyn) {
  if (shdr.sh_entsize == 0) shdr.sh_entsize = sizeof(PackedElfDyn<B, E>);
  auto ret = entry_count();
  data.insert(data.end(), reinterpret_cast<const u8 *>(&dyn),
              reinterpret_cast<const u8 *>(&dyn) + sizeof(PackedElfDyn<B, E>));
  shdr.sh_size = data.size();
  return ret;
}

template <ElfBits B, ElfEndian E, bool Const> inline u32 PackedDynamicAccessor<B, E, Const>::add_entry(word<B> tag) {
  return add_entry(tag, 0);
}

template <ElfBits B, ElfEndian E, bool Const>
u32 PackedDynamicAccessor<B, E, Const>::add_entry(word<B> tag, word<B> value) {
  PackedElfDyn<B, E> dyn;
  dyn.d_tag = tag;
  dyn.d_val = value;
  return add_entry(std::move(dyn));
}

template <ElfBits B, ElfEndian E, bool Const> u32 PackedDynamicAccessor<B, E, Const>::add_entry(DynamicTags tag) {
  return add_entry(to_underlying(tag));
}

template <ElfBits B, ElfEndian E, bool Const>
u32 PackedDynamicAccessor<B, E, Const>::add_entry(DynamicTags tag, word<B> value) {
  return add_entry(to_underlying(tag), value);
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedDynamicAccessor<B, E, Const>::replace_entry(u32 index, PackedElfDyn<B, E> &&dyn) noexcept {
  if (auto ptr = get_entry_ptr(index); ptr != nullptr) {
    ptr->d_tag = dyn.d_tag;
    ptr->d_val = dyn.d_val;
  }
}

template <ElfBits B, ElfEndian E, bool Const>
void PackedDynamicAccessor<B, E, Const>::replace_entry(u32 index, word<B> tag, word<B> value) noexcept {
  if (auto ptr = get_entry_ptr(index); ptr != nullptr) {
    ptr->d_tag = tag;
    ptr->d_val = value;
  }
}

template <ElfBits B, ElfEndian E, bool Const>
AbsoluteFixup PackedDynamicAccessor<B, E, Const>::fixup_value(u32 index, std::function<word<B>()> f) {
  return AbsoluteFixup([this, index, f]() {
    auto ptr = get_entry_ptr(index);
    if (ptr != nullptr) ptr->d_val = f();
  });
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
