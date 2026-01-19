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
#include "core/elf/packed_access_array.hpp"
#include "core/elf/packed_access_dynamic.hpp"
#include "core/elf/packed_access_hash.hpp"
#include "core/elf/packed_access_note.hpp"
#include "core/elf/packed_access_relocations.hpp"
#include "core/elf/packed_access_symbol.hpp"
#include "core/elf/packed_elf.hpp"
#include "core/elf/packed_fixup.hpp"
#include "core/elf/packed_ops.hpp"
#include "core/elf/packed_types.hpp"

namespace {
bool write(const std::string &fname, const std::span<const u8> &data) {
  std::ofstream out(fname, std::ios::binary);
  if (!out.is_open()) return false;
  out.write(reinterpret_cast<const char *>(data.data()), data.size());
  return out.good();
}
template <pepp::bts::ElfBits B, pepp::bts::ElfEndian E>
void do_version(pepp::bts::ElfMachineType t, std::string fname) {
  using namespace pepp::bts;
  using enum DynamicTags;
  using Packed = PackedGrowableElfFile<B, E>;

  Packed elf(ElfFileType::ET_EXEC, t, ElfABI::ELFOSABI_NONE);
  ensure_section_header_table(elf);

  std::deque<AbsoluteFixup> fixups;
  auto dynstr_idx = add_named_section(elf, ".dynstr", SectionTypes::SHT_STRTAB);
  auto dynsym_idx = add_named_dynsymtab(elf, ".dynsym", dynstr_idx);
  auto versym_idx = add_gnu_version(elf, dynsym_idx);
  {
    PackedSymbolWriter<B, E> st_writer(elf, dynsym_idx);
    st_writer.add_symbol(create_global_symbol<B, E>(), "alpha", 0);
    st_writer.add_symbol(create_global_symbol<B, E>(), "bravo", 0);
    st_writer.add_symbol(create_global_symbol<B, E>(), "charlie", 0);
    st_writer.add_symbol(create_global_symbol<B, E>(), "delta", 0);
    st_writer.arrange_local_symbols();
  }
  {
    PackedSymbolVersionWriter<B, E> vs_writer(elf, versym_idx);
    vs_writer.set_version(0, 0x0000); // alpha
    vs_writer.set_version(1, 0x8001); // bravo
    vs_writer.set_version(2, 0x8001); // charlie
    vs_writer.set_version(3, 0xFEDE); // delta
  }

  auto layout = calculate_layout(elf);
  for (const auto &fixup : fixups) fixup.update();
  std::vector<u8> data(size_for_layout(layout), 0);
  write(data, layout);
  write(fname, data);
}
template <pepp::bts::ElfBits B, pepp::bts::ElfEndian E>
void do_version_rd(pepp::bts::ElfMachineType t, std::string fname) {
  using namespace pepp::bts;
  using enum DynamicTags;
  using Packed = PackedGrowableElfFile<B, E>;
  Packed elf(ElfFileType::ET_EXEC, t, ElfABI::ELFOSABI_NONE);
  ensure_section_header_table(elf);

  std::deque<AbsoluteFixup> fixups;
  auto dynstr_idx = add_named_section(elf, ".dynstr", SectionTypes::SHT_STRTAB);
  auto dynsym_idx = add_named_dynsymtab(elf, ".dynsym", dynstr_idx);
  auto versym_idx = add_gnu_version(elf, dynsym_idx);
  auto versymr_idx = add_gnu_version_r(elf, dynstr_idx);
  auto versymd_idx = add_gnu_version_d(elf, dynstr_idx);
  PackedStringWriter<B, E> st_writer(elf, dynstr_idx);

  auto gl_sm = st_writer.add_string("__libc_start_main");
  auto gl_fin = st_writer.add_string("__cxa_finalize");
  auto gl_pf = st_writer.add_string("__printf_chk");
  auto gl_ab = st_writer.add_string("abort");
  auto ex_gl = st_writer.add_string("_rtld_global_ro");   //@@GLIBC_PRIVATE
  auto ex_rs = st_writer.add_string("__rseq_flags");      //@@GLIBC_2.35
  auto ex_st = st_writer.add_string("__stack_chk_guard"); //@@GLIBC_2.17
  auto libc = st_writer.add_string("libc.so.6");
  auto v217 = st_writer.add_string("GLIBC_2.17");
  auto v234 = st_writer.add_string("GLIBC_2.34");
  auto ldso = st_writer.add_string("ld-linux-aarch64.so.1");
  auto v235 = st_writer.add_string("GLIBC_2.35");
  auto vpriv = st_writer.add_string("GLIBC_PRIVATE");

  {
    PackedSymbolWriter<B, E> st_writer(elf, dynsym_idx);
    auto create_import = [](u32 name) {
      auto r = create_global_symbol<B, E>();
      r.st_name = name;
      return r;
    };
    auto create_export = [](u32 name) {
      auto r = create_global_symbol<B, E>();
      r.st_name = name;
      r.st_shndx = 1; // Some valid section
      return r;
    };

    st_writer.add_symbol(create_import(gl_sm));
    st_writer.add_symbol(create_import(gl_fin));
    st_writer.add_symbol(create_import(gl_pf));
    st_writer.add_symbol(create_import(gl_ab));
    st_writer.add_symbol(create_export(ex_gl));
    st_writer.add_symbol(create_export(ex_rs));
    st_writer.add_symbol(create_export(ex_st));
    st_writer.arrange_local_symbols();
  }
  {
    PackedSymbolVersionWriter<B, E> vs_writer(elf, versym_idx);
    vs_writer.set_version(0, 0x0000);
    vs_writer.set_version(1, 0x0003); // __libc_start_main
    vs_writer.set_version(2, 0x0002); // __cxa_finalize
    vs_writer.set_version(3, 0x0002); // __printf_chk
    vs_writer.set_version(4, 0x0002); // abort
    vs_writer.set_version(5, 0x0005); // _rtld_global_ro
    vs_writer.set_version(6, 0x0004); // __rseq_flags
    vs_writer.set_version(7, 0x0002); // __stack_chk_guard
  }

  {
    PackedVersionNeedWriter<B, E> vs_writer(elf, versymr_idx);
    PackedElfVerneed<E> verneed1;
    verneed1.vn_version = 1;
    verneed1.vn_file = libc;
    verneed1.vn_cnt = 2;
    vs_writer.add_ver(std::move(verneed1));
    PackedElfVernaux<E> vernaux1;
    vernaux1.vna_hash = elf_hash(std::span{"GLIBC_2.17", 10});
    vernaux1.vna_flags = 0;
    vernaux1.vna_other = 2;
    vernaux1.vna_name = v217;
    vs_writer.add_aux(0, std::move(vernaux1));
    PackedElfVernaux<E> vernaux2;
    vernaux2.vna_hash = elf_hash(std::span{"GLIBC_2.34", 10});
    vernaux2.vna_flags = 0;
    vernaux2.vna_other = 3;
    vernaux2.vna_name = v234;
    vs_writer.add_aux(0, std::move(vernaux2));
  }

  {
    // Using a handful of symbols from /lib/ld-linux-aarch64.so.1 and all of the verdef entries.
    PackedVersionDefWriter<B, E> vd_writer(elf, versymd_idx);
    auto add_def = [&](std::uint16_t ndx, std::uint16_t flags, std::uint16_t cnt, std::uint32_t name_off,
                       std::string_view name_sv) {
      PackedElfVerdef<E> vd{};
      vd.vd_version = 1;
      vd.vd_flags = flags;
      vd.vd_ndx = ndx;
      vd.vd_cnt = cnt;
      vd.vd_hash = elf_hash(std::span{name_sv.data(), name_sv.size()});
      // vd_aux / vd_next should be filled by the writer (or you must patch them)
      vd_writer.add_ver(std::move(vd));
    };

    auto add_aux = [&](std::size_t def_index, std::uint32_t name_off) {
      PackedElfVerdaux<E> a{};
      a.vda_name = name_off;
      vd_writer.add_aux(def_index, std::move(a));
    };

    // 0) BASE: Index 1, Cnt 1, Name ld-linux-aarch64.so.1
    add_def(1, 1, 1, ldso, "ld-linux-aarch64.so.1");
    add_aux(0, ldso);

    // 1) GLIBC_2.17: Index 2, Cnt 1, Name GLIBC_2.17
    add_def(2, 0, 1, v217, "GLIBC_2.17");
    add_aux(1, v217);

    // 2) GLIBC_2.34: Index 3, Cnt 2, Name GLIBC_2.34, Parent 1 GLIBC_2.17
    add_def(3, 0, 2, v234, "GLIBC_2.34");
    add_aux(2, v234); // Name
    add_aux(2, v217); // Parent 1

    // 3) GLIBC_2.35: Index 4, Cnt 2, Name GLIBC_2.35, Parent 1 GLIBC_2.34
    add_def(4, 0, 2, v235, "GLIBC_2.35");
    add_aux(3, v235); // Name
    add_aux(3, v234); // Parent 1

    // 4) GLIBC_PRIVATE: Index 5, Cnt 2, Name GLIBC_PRIVATE, Parent 1 GLIBC_2.35
    add_def(5, 0, 2, vpriv, "GLIBC_PRIVATE");
    add_aux(4, vpriv); // Name
    add_aux(4, v235);  // Parent 1
  }

  auto layout = calculate_layout(elf);
  for (const auto &fixup : fixups) fixup.update();
  std::vector<u8> data(size_for_layout(layout), 0);
  write(data, layout);
  write(fname, data);
}
} // namespace

TEST_CASE("Emit .gnu.version* sections", "[scope:elf][kind:unit][arch:*]") {
  using namespace pepp::bts;
  using Packed = PackedElfLE32;
  SECTION("Emit a working 32-bit, little-endian .gnu.version") {
    do_version<ElfBits::b32, ElfEndian::le>(ElfMachineType::EM_386, "compat_versym_32le.elf");
  }
  SECTION("Emit a working 32-bit big-endian .gnu.version") {
    do_version<ElfBits::b32, ElfEndian::be>(ElfMachineType::EM_386, "compat_versym_32be.elf");
  }
  SECTION("Emit a working 32-bit, little-endian .gnu.version_r and .gnu.version_d") {
    do_version_rd<ElfBits::b32, ElfEndian::le>(ElfMachineType::EM_ARM, "compat_verrd_32le.elf");
  }
  SECTION("Emit a working 32-bit, big-endian .gnu.version_r and .gnu.version_d") {
    do_version_rd<ElfBits::b32, ElfEndian::be>(ElfMachineType::EM_ARM, "compat_verrd_32be.elf");
  }
  SECTION("Emit a working 64-bit, little-endian .gnu.version") {
    do_version<ElfBits::b64, ElfEndian::le>(ElfMachineType::EM_386, "compat_versym_64le.elf");
  }
  SECTION("Emit a working 64-bit big-endian .gnu.version") {
    do_version<ElfBits::b64, ElfEndian::be>(ElfMachineType::EM_386, "compat_versym_64be.elf");
  }
  SECTION("Emit a working 64-bit, little-endian .gnu.version_r and .gnu.version_d") {
    do_version_rd<ElfBits::b64, ElfEndian::le>(ElfMachineType::EM_ARM, "compat_verrd_64le.elf");
  }
  SECTION("Emit a working 64-bit, big-endian .gnu.version_r and .gnu.version_d") {
    do_version_rd<ElfBits::b64, ElfEndian::be>(ElfMachineType::EM_ARM, "compat_verrd_64be.elf");
  }
}
