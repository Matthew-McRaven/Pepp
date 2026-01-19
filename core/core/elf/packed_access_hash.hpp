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
#include <numeric>
#include "core/bitmanip/log2.hpp"
#include "core/elf/packed_access_symbol.hpp"
#include "core/elf/packed_elf.hpp"

namespace pepp::bts {
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
  using Data = maybe_const_t<Const, std::shared_ptr<AStorage>>;
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
  using Data = maybe_const_t<Const, std::shared_ptr<AStorage>>;
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
  for (i32 i = hashed_count - 1; i > 0; i--) {
    u32 b = elf_hash(this->get_symbol_name(i)) % nbuckets;
    chains[i] = buckets[b]; // next is previous head (0 if empty)
    buckets[b] = i;         // new head is this symbol
  }

  // Write out the hash table in correct endian format. Avoid reallocations of underlying buffer.
  data_hash->clear(2 * sizeof(u32) + +nbuckets * sizeof(u32) + chains.size() * sizeof(u32));

  // Helper to conditionally byteswap and append a u32
  auto append_u32 = [](auto &data_hash, u32 val) {
    u8 buf[4];
    *((U32<E> *)buf) = val;
    data_hash->append(std::span<const u8>{buf});
  };

  // Header, buckets, chains
  append_u32(data_hash, nbuckets);
  append_u32(data_hash, chains.size());
  for (const auto &b : buckets) append_u32(data_hash, b);
  for (const auto &c : chains) append_u32(data_hash, c);
}

template <ElfBits B, ElfEndian E, bool Const> u32 PackedHashedSymbolAccessor<B, E, Const>::nbuckets() const noexcept {
  if (data_hash->size() < 2 * sizeof(u32)) return 0;
  return *data_hash->template get<U32<E>>(0);
}

template <ElfBits B, ElfEndian E, bool Const> u32 PackedHashedSymbolAccessor<B, E, Const>::nchains() const noexcept {
  if (data_hash->size() < 2 * sizeof(u32)) return 0;
  return *data_hash->template get<U32<E>>(1);
}

template <ElfBits B, ElfEndian E, bool Const>
bits::span<const U32<E>> PackedHashedSymbolAccessor<B, E, Const>::buckets() const noexcept {
  const auto offset = 2 * sizeof(u32);
  if (data_hash->size() < offset) return {};
  bits::span<const u8> underlying = data_hash->get(offset, nbuckets() * sizeof(u32));
  return bits::span<const U32<E>>{(const U32<E> *)underlying.data(), nbuckets()};
}

template <ElfBits B, ElfEndian E, bool Const>
bits::span<const U32<E>> PackedHashedSymbolAccessor<B, E, Const>::chains() const noexcept {
  const auto offset = (2 + nbuckets()) * sizeof(u32);
  if (data_hash->size() < offset) return {};
  bits::span<const u8> underlying = data_hash->get(offset, nchains() * sizeof(u32));
  return bits::span<const U32<E>>{(const U32<E> *)underlying.data(), nchains()};
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
  static constexpr u32 WordBits = 8u * sizeof(Word<B, E>);
  const u32 bloom_size = this->maskwords();
  const u32 bloom_shift = this->mshift2();
  const auto nbuckets = this->nbuckets();
  const auto bloom_filter = this->bloom();
  u32 hash = gnu_elf_hash(name);
  u32 bloom_index = (hash / WordBits) % bloom_size;
  word<B> bloom_bits = ((word<B>)1 << (hash % (WordBits))) | ((word<B>)1 << ((hash >> bloom_shift) % (WordBits)));
  word<B> stored_bits = bloom_filter[bloom_index];
  auto combined = stored_bits & bloom_bits;
  if (combined != bloom_bits) return 0;

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
  constexpr u32 WordBits = 8u * sizeof(word<B>);
  for (u32 i = 0; i < hashed_count; ++i) {
    const word<B> H1 = H[i], H2 = (H1 >> shift2);
    const word<B> N = ((H1 / WordBits) & maskwords_bitmask);
    const word<B> bitmask = (word<B>{1} << (H1 % WordBits)) | (word<B>{1} << (H2 % WordBits));
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
  data_hash->clear((4 + nbuckets + chains.size()) * sizeof(u32) + maskwords * sizeof(word<B>));

  // Helper to conditionally byteswap and append a u32
  auto append_u32 = [](auto &data_hash, u32 val) {
    u8 buf[4];
    *((U32<E> *)buf) = val;
    data_hash->append(std::span<const u8>{buf});
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
    data_hash->append(std::span<const u8>{buf});
  }
  for (const auto &b : buckets) append_u32(data_hash, b);
  for (const auto &c : chains) append_u32(data_hash, c);
}

template <ElfBits B, ElfEndian E, bool Const>
u32 PackedGNUHashedSymbolAccessor<B, E, Const>::nbuckets() const noexcept {
  if (data_hash->size() < 4 * sizeof(u32)) return 0;
  return *data_hash->template get<U32<E>>(0);
}

template <ElfBits B, ElfEndian E, bool Const> u32 PackedGNUHashedSymbolAccessor<B, E, Const>::symndx() const noexcept {
  if (data_hash->size() < 4 * sizeof(u32)) return 0;
  return *data_hash->template get<U32<E>>(1);
}

template <ElfBits B, ElfEndian E, bool Const>
u32 PackedGNUHashedSymbolAccessor<B, E, Const>::maskwords() const noexcept {
  if (data_hash->size() < 4 * sizeof(u32)) return 0;
  return *data_hash->template get<U32<E>>(2);
}

template <ElfBits B, ElfEndian E, bool Const> u32 PackedGNUHashedSymbolAccessor<B, E, Const>::mshift2() const noexcept {
  if (data_hash->size() < 4 * sizeof(u32)) return 0;
  return *data_hash->template get<U32<E>>(3);
}

template <ElfBits B, ElfEndian E, bool Const>
bits::span<const Word<B, E>> PackedGNUHashedSymbolAccessor<B, E, Const>::bloom() const noexcept {
  const auto offset = 4 * sizeof(u32);
  if (data_hash->size() < offset) return {};
  bits::span<const u8> underlying = data_hash->get(offset, maskwords() * sizeof(Word<B, E>));
  return bits::span<const Word<B, E>>{(const Word<B, E> *)underlying.data(), maskwords()};
}

template <ElfBits B, ElfEndian E, bool Const>
bits::span<const U32<E>> PackedGNUHashedSymbolAccessor<B, E, Const>::buckets() const noexcept {
  const auto offset = 4 * sizeof(u32) + maskwords() * sizeof(Word<B, E>);
  if (data_hash->size() < offset) return {};
  bits::span<const u8> underlying = data_hash->get(offset, nbuckets() * sizeof(u32));
  return bits::span<const U32<E>>{(const U32<E> *)underlying.data(), nbuckets()};
}

template <ElfBits B, ElfEndian E, bool Const>
bits::span<const U32<E>> PackedGNUHashedSymbolAccessor<B, E, Const>::chains() const noexcept {
  const auto offset = 4 * sizeof(u32) + maskwords() * sizeof(Word<B, E>) + nbuckets() * sizeof(u32);
  if (data_hash->size() < offset) return {};
  bits::span<const u8> underlying = data_hash->get(offset, data_hash->size() - offset);
  return bits::span<const U32<E>>{(const U32<E> *)underlying.data(), nbuckets()};
}
} // namespace pepp::core
