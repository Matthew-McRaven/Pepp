/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
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
#include <catch.hpp>
#include <elfio/elfio.hpp>
#include <functional>
#include "bts/elf/packed_access_array.hpp"
#include "bts/elf/packed_access_dynamic.hpp"
#include "bts/elf/packed_access_hash.hpp"
#include "bts/elf/packed_access_note.hpp"
#include "bts/elf/packed_access_relocations.hpp"
#include "bts/elf/packed_access_symbol.hpp"
#include "bts/elf/packed_elf.hpp"
#include "bts/elf/packed_fixup.hpp"
#include "bts/elf/packed_ops.hpp"
#include "bts/elf/packed_types.hpp"

namespace {
bool write(const std::string &fname, const std::span<const u8> &data) {
  std::ofstream out(fname, std::ios::binary);
  if (!out.is_open()) return false;
  out.write(reinterpret_cast<const char *>(data.data()), data.size());
  return out.good();
}
template <pepp::bts::ElfBits B, pepp::bts::ElfEndian E> void do_hash(pepp::bts::ElfMachineType t, std::string fname) {
  using namespace pepp::bts;
  using enum DynamicTags;
  using Packed = PackedGrowableElfFile<B, E>;

  Packed elf(ElfFileType::ET_EXEC, t, ElfABI::ELFOSABI_NONE);
  ensure_section_header_table(elf);
  elf.add_segment(SegmentType::PT_DYNAMIC);                                       // dynamic
  elf.add_segment(SegmentType::PT_LOAD, SegmentFlags::PF_R | SegmentFlags::PF_W); // load

  std::deque<AbsoluteFixup> fixups;
  auto symtab_idx = add_named_symtab(elf, ".symtab", add_named_section(elf, ".strtab", SectionTypes::SHT_STRTAB));
  auto dynstr_idx = add_named_section(elf, ".dynstr", SectionTypes::SHT_STRTAB);
  auto dynsym_idx = add_named_dynsymtab(elf, ".dynsym", dynstr_idx);
  auto hash_idx = add_named_section(elf, ".hash", SectionTypes::SHT_HASH, dynsym_idx);
  auto dynamic_idx = add_named_section(elf, ".dynamic", SectionTypes::SHT_DYNAMIC, symtab_idx);

  {
    PackedHashedSymbolWriter<B, E> hs_writer(elf, hash_idx);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "alpha", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "bravo", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "charlie", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "delta", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "echo", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "foxtrot", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "golf", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "hotel", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "india", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "juliett", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "kilo", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "lima", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "mike", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "november", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "oscar", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "papa", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "quebec", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "romeo", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "sierra", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "tango", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "uniform", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "victor", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "whiskey", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "x-ray", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "yankee", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "zulu", 1);
    hs_writer.arrange_local_symbols();
    hs_writer.compute_hash_table(13);
  }
  {
    PackedSymbolWriter<B, E> st_writer(elf, symtab_idx);
    auto _DYNAMIC = st_writer.add_symbol(create_null_symbol<B, E>(), "_DYNAMIC", dynamic_idx);
    st_writer.arrange_local_symbols();
    fixups.emplace_back(
        fixup_symbol_value(elf, dynamic_idx, _DYNAMIC, [&]() { return elf.program_headers[0].p_paddr; }));
  }
  {
    PackedDynamicWriter<B, E> dyn_writer(elf, dynamic_idx);
    fixups.emplace_back(fixup_dynamic_value(elf, dynamic_idx, dyn_writer.add_entry(DT_STRTAB),
                                            [&]() { return elf.section_headers[dynstr_idx].sh_addr; }));
    fixups.emplace_back(fixup_dynamic_value(elf, dynamic_idx, dyn_writer.add_entry(DT_STRSZ),
                                            [&]() { return elf.section_headers[dynstr_idx].sh_size; }));
    fixups.emplace_back(fixup_dynamic_value(elf, dynamic_idx, dyn_writer.add_entry(DT_SYMTAB),
                                            [&]() { return elf.section_headers[dynsym_idx].sh_addr; }));
    fixups.emplace_back(fixup_dynamic_value(elf, dynamic_idx, dyn_writer.add_entry(DT_HASH),
                                            [&]() { return elf.section_headers[hash_idx].sh_addr; }));
    dyn_writer.add_entry(DT_NULL, 0);
  }

  SegmentLayoutConstraint constraint_dyn;
  constraint_dyn.alignment = 4096;
  constraint_dyn.from_sec = dynstr_idx;
  constraint_dyn.to_sec = dynamic_idx;
  constraint_dyn.base_address = 0xFEED;
  SegmentLayoutConstraint constraint_load = constraint_dyn;
  constraint_load.update_sec_addrs = true;
  std::vector<SegmentLayoutConstraint> constraints = {constraint_dyn, constraint_load};

  auto layout = calculate_layout(elf, &constraints);
  for (const auto &fixup : fixups) fixup.update();
  std::vector<u8> data(size_for_layout(layout), 0);
  write(data, layout);
  write(fname, data);

  PackedInputElfFile<B, E> in(fname);
  // Easiest way to verify manually is download llvm for llvm-readelf and run:
  // llvm-readelf-15 --gnu-hash-table -a --hash-symbols -d --dyn-syms ehdr_hash.elf
  PackedHashedSymbolReader<B, E> hs_reader(in, hash_idx);
  CHECK(hs_reader.find_hashed_symbol("alpha") == 1);
  CHECK(hs_reader.find_hashed_symbol("bravo") == 2);
  CHECK(hs_reader.find_hashed_symbol("charlie") == 3);
  CHECK(hs_reader.find_hashed_symbol("delta") == 4);
  CHECK(hs_reader.find_hashed_symbol("echo") == 5);
  CHECK(hs_reader.find_hashed_symbol("foxtrot") == 6);
  CHECK(hs_reader.find_hashed_symbol("golf") == 7);
  CHECK(hs_reader.find_hashed_symbol("hotel") == 8);
  CHECK(hs_reader.find_hashed_symbol("india") == 9);
  CHECK(hs_reader.find_hashed_symbol("juliett") == 10);
  CHECK(hs_reader.find_hashed_symbol("kilo") == 11);
  CHECK(hs_reader.find_hashed_symbol("lima") == 12);
  CHECK(hs_reader.find_hashed_symbol("mike") == 13);
  CHECK(hs_reader.find_hashed_symbol("november") == 14);
  CHECK(hs_reader.find_hashed_symbol("oscar") == 15);
  CHECK(hs_reader.find_hashed_symbol("papa") == 16);
  CHECK(hs_reader.find_hashed_symbol("quebec") == 17);
  CHECK(hs_reader.find_hashed_symbol("romeo") == 18);
  CHECK(hs_reader.find_hashed_symbol("sierra") == 19);
  CHECK(hs_reader.find_hashed_symbol("tango") == 20);
  CHECK(hs_reader.find_hashed_symbol("uniform") == 21);
  CHECK(hs_reader.find_hashed_symbol("victor") == 22);
  CHECK(hs_reader.find_hashed_symbol("whiskey") == 23);
  CHECK(hs_reader.find_hashed_symbol("x-ray") == 24);
  CHECK(hs_reader.find_hashed_symbol("yankee") == 25);
  CHECK(hs_reader.find_hashed_symbol("zulu") == 26);
}
template <pepp::bts::ElfBits B, pepp::bts::ElfEndian E>
void do_gnuhash(pepp::bts::ElfMachineType t, std::string fname) {
  using namespace pepp::bts;
  using enum DynamicTags;
  using Packed = PackedGrowableElfFile<B, E>;

  Packed elf(ElfFileType::ET_EXEC, t, ElfABI::ELFOSABI_NONE);
  ensure_section_header_table(elf);
  elf.add_segment(SegmentType::PT_DYNAMIC);                                       // dynamic
  elf.add_segment(SegmentType::PT_LOAD, SegmentFlags::PF_R | SegmentFlags::PF_W); // load

  std::deque<AbsoluteFixup> fixups;
  auto symtab_idx = add_named_symtab(elf, ".symtab", add_named_section(elf, ".strtab", SectionTypes::SHT_STRTAB));
  auto dynstr_idx = add_named_section(elf, ".dynstr", SectionTypes::SHT_STRTAB);
  auto dynsym_idx = add_named_dynsymtab(elf, ".dynsym", dynstr_idx);
  auto hash_idx = add_named_section(elf, ".gnu.hash", SectionTypes::SHT_GNU_HASH, dynsym_idx);
  auto dynamic_idx = add_named_section(elf, ".dynamic", SectionTypes::SHT_DYNAMIC);

  {
    PackedGNUHashedSymbolWriter<B, E> hs_writer(elf, hash_idx);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "alpha", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "bravo", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "charlie", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "delta", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "echo", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "foxtrot", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "golf", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "hotel", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "india", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "juliett", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "kilo", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "lima", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "mike", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "november", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "oscar", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "papa", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "quebec", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "romeo", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "sierra", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "tango", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "uniform", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "victor", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "whiskey", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "x-ray", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "yankee", 1);
    hs_writer.add_symbol(create_global_symbol<B, E>(), "zulu", 1);
    hs_writer.arrange_local_symbols();
    hs_writer.compute_hash_table(11, 4, 2, 5);
  }
  {
    PackedSymbolWriter<B, E> st_writer(elf, symtab_idx);
    auto _DYNAMIC = st_writer.add_symbol(create_null_symbol<B, E>(), "_DYNAMIC", dynamic_idx);
    st_writer.arrange_local_symbols();
    fixups.emplace_back(
        fixup_symbol_value(elf, dynamic_idx, _DYNAMIC, [&]() { return elf.program_headers[0].p_paddr; }));
  }
  {
    PackedDynamicWriter<B, E> dyn_writer(elf, dynamic_idx);

    fixups.emplace_back(fixup_dynamic_value(elf, dynamic_idx, dyn_writer.add_entry(DT_STRTAB),
                                            [&]() { return elf.section_headers[dynstr_idx].sh_addr; }));
    fixups.emplace_back(fixup_dynamic_value(elf, dynamic_idx, dyn_writer.add_entry(DT_STRSZ),
                                            [&]() { return elf.section_headers[dynstr_idx].sh_size; }));
    fixups.emplace_back(fixup_dynamic_value(elf, dynamic_idx, dyn_writer.add_entry(DT_SYMTAB),
                                            [&]() { return elf.section_headers[dynsym_idx].sh_addr; }));
    fixups.emplace_back(fixup_dynamic_value(elf, dynamic_idx, dyn_writer.add_entry(DT_GNU_HASH),
                                            [&]() { return elf.section_headers[hash_idx].sh_addr; }));
    dyn_writer.add_entry(DT_NULL, 0);
  }
  SegmentLayoutConstraint constraint_dyn;
  constraint_dyn.alignment = 4096;
  constraint_dyn.from_sec = dynstr_idx;
  constraint_dyn.to_sec = dynamic_idx;
  constraint_dyn.base_address = 0xFEED;
  SegmentLayoutConstraint constraint_load = constraint_dyn;
  constraint_load.update_sec_addrs = true;
  std::vector<SegmentLayoutConstraint> constraints = {constraint_dyn, constraint_load};

  auto layout = calculate_layout(elf, &constraints);
  for (const auto &fixup : fixups) fixup.update();
  std::vector<u8> data(size_for_layout(layout), 0);
  write(data, layout);
  write(fname, data);

  PackedInputElfFile<B, E> in(fname);

  // Assuming nbuckets=11, symndx=mask_words=11, shift2=5, symndx=4 (start hash at delta)
  // Easiest way to verify manually is download llvm for llvm-readelf and run:
  // llvm-readelf-15 --gnu-hash-table -a --hash-symbols -d --dyn-syms ehdr_gnuhash.elf
  PackedGNUHashedSymbolReader<B, E> hs_reader(in, hash_idx);
  CHECK(hs_reader.find_hashed_symbol("alpha") == 0);
  CHECK(hs_reader.find_hashed_symbol("bravo") == 0);
  CHECK(hs_reader.find_hashed_symbol("charlie") == 0);
  CHECK(hs_reader.find_hashed_symbol("delta") == 4);
  CHECK(hs_reader.find_hashed_symbol("Delta") == 0);
  CHECK(hs_reader.find_hashed_symbol("hotel") == 5);
  CHECK(hs_reader.find_hashed_symbol("india") == 6);
  CHECK(hs_reader.find_hashed_symbol("sierra") == 7);
  CHECK(hs_reader.find_hashed_symbol("x-ray") == 8);
  CHECK(hs_reader.find_hashed_symbol("quebec") == 9);
  CHECK(hs_reader.find_hashed_symbol("romeo") == 10);
  CHECK(hs_reader.find_hashed_symbol("tango") == 11);
  CHECK(hs_reader.find_hashed_symbol("zulu") == 12);
  CHECK(hs_reader.find_hashed_symbol("lima") == 13);
  CHECK(hs_reader.find_hashed_symbol("papa") == 14);
  CHECK(hs_reader.find_hashed_symbol("uniform") == 15);
  CHECK(hs_reader.find_hashed_symbol("yankee") == 16);
  CHECK(hs_reader.find_hashed_symbol("juliett") == 17);
  CHECK(hs_reader.find_hashed_symbol("oscar") == 18);
  CHECK(hs_reader.find_hashed_symbol("whiskey") == 19);
  CHECK(hs_reader.find_hashed_symbol("victor") == 20);
  CHECK(hs_reader.find_hashed_symbol("echo") == 21);
  CHECK(hs_reader.find_hashed_symbol("kilo") == 22);
  CHECK(hs_reader.find_hashed_symbol("mike") == 23);
  CHECK(hs_reader.find_hashed_symbol("foxtrot") == 24);
  CHECK(hs_reader.find_hashed_symbol("golf") == 25);
  CHECK(hs_reader.find_hashed_symbol("november") == 26);
}
} // namespace

TEST_CASE("Emit .gnu.hash and .hash sections", "[scope:elf][kind:unit][arch:*]") {
  using namespace pepp::bts;
  using Packed = PackedElfLE32;
  SECTION("Validate hash functions for known values") {
    // Sample hashes from: https://flapenguin.me/elf-dt-hash
    CHECK(elf_hash(std::span{"", 0}) == 0);
    CHECK(elf_hash(std::span{"printf", 6}) == 0x077905a6);
    CHECK(elf_hash(std::span{"exit", 4}) == 0x0006cf04);
    CHECK(elf_hash(std::span{"syscall", 7}) == 0x0b09985c);
    CHECK(elf_hash(std::span{"flapenguin.me", 13}) == 0x03987915);

    // Do not include null terminators for sake of matching existing hashes
    CHECK(gnu_elf_hash(std::span{"", 0}) == 0x00001505);
    CHECK(gnu_elf_hash(std::span{"printf", 6}) == 0x156b2bb8);
    CHECK(gnu_elf_hash(std::span{"exit", 4}) == 0x7c967e3f);
    CHECK(gnu_elf_hash(std::span{"syscall", 7}) == 0xbac212a0);
    CHECK(gnu_elf_hash(std::span{"flapenguin.me", 13}) == 0x8ae9f18e);
  }
  SECTION("Emit a working 32-bit, little-endian .hash") {
    do_hash<ElfBits::b32, ElfEndian::le>(ElfMachineType::EM_386, "compat_hash_32le.elf");
  }
  SECTION("Emit a working 32-bit big-endian .hash") {
    do_hash<ElfBits::b32, ElfEndian::be>(ElfMachineType::EM_386, "compat_hash_32be.elf");
  }
  SECTION("Emit a working 32-bit, little-endian .gnu.hash") {
    do_gnuhash<ElfBits::b32, ElfEndian::le>(ElfMachineType::EM_ARM, "compat_gnuhash_32le.elf");
  }
  SECTION("Emit a working 32-bit, big-endian .gnu.hash") {
    do_gnuhash<ElfBits::b32, ElfEndian::be>(ElfMachineType::EM_ARM, "compat_gnuhash_32be.elf");
  }
  SECTION("Emit a working 64-bit, little-endian .hash") {
    do_hash<ElfBits::b64, ElfEndian::le>(ElfMachineType::EM_386, "compat_hash_64le.elf");
  }
  SECTION("Emit a working 64-bit big-endian .hash") {
    do_hash<ElfBits::b64, ElfEndian::be>(ElfMachineType::EM_386, "compat_hash_64be.elf");
  }
  SECTION("Emit a working 64-bit, little-endian .gnu.hash") {
    do_gnuhash<ElfBits::b64, ElfEndian::le>(ElfMachineType::EM_ARM, "compat_gnuhash_64le.elf");
  }
  SECTION("Emit a working 64-bit, big-endian .gnu.hash") {
    do_gnuhash<ElfBits::b64, ElfEndian::be>(ElfMachineType::EM_ARM, "compat_gnuhash_64be.elf");
  }
}
