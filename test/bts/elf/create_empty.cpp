/*
 * Copyright (c) 2023-2025 J. Stanley Warford, Matthew McRaven
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
#include "bts/elf/packed_access.hpp"
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
} // namespace

TEST_CASE("Test custom ELF library, 32-bit", "[scope:elf][kind:unit][arch:*]") {
  using namespace pepp::bts;
  using Packed = PackedElfLE32;
  SECTION("Create ehdr with custom, read with ELFIO") {
    auto my_header = Packed::Ehdr(ElfFileType::ET_EXEC, ElfMachineType::EM_PEP10, ElfABI::ELFOSABI_NONE);
    auto data = bits::span<const u8>{reinterpret_cast<const u8 *>(&my_header), sizeof(my_header)};
    CHECK(data.size() == 52);
    write("ehdr_only_test32.elf", data);
    ELFIO::elfio elf;
    std::istringstream in(std::string((const char *)data.data(), data.size()));
    CHECK_NOTHROW(elf.load(in) == true);
    CHECK(elf.get_class() == ELFIO::ELFCLASS32);
  }
  SECTION("Create ehdr with shdr table") {
    Packed elf(ElfFileType::ET_EXEC, ElfMachineType::EM_PEP8, ElfABI::ELFOSABI_NONE);
    ensure_section_header_table(elf);
    auto layout = calculate_layout(elf);
    std::vector<u8> data(size_for_layout(layout), 0);
    write(data, layout);
    write("ehdr_shdr32.elf", data);
    ELFIO::elfio elfio;
    std::istringstream in(std::string((const char *)data.data(), data.size()));
    CHECK_NOTHROW(elfio.load(in) == true);
    CHECK(elfio.get_class() == ELFIO::ELFCLASS32);
    CHECK(elfio.sections.size() == 2);
  }
  SECTION("Create shdr table by hand") {
    Packed elf(ElfFileType::ET_EXEC, ElfMachineType::EM_PEP8, ElfABI::ELFOSABI_NONE);
    elf.add_section(create_null_header<ElfBits::b32, ElfEndian::le>());
    auto shstrtab_idx = elf.add_section(create_shstrtab_header<ElfBits::b32, ElfEndian::le>(1));
    PackedStringWriter<ElfBits::b32, ElfEndian::le> writer(elf.section_headers[shstrtab_idx],
                                                           elf.section_data[shstrtab_idx]);
    elf.header.e_shstrndx = shstrtab_idx;
    elf.section_headers[shstrtab_idx].sh_name = writer.add_string(".shstrtab");

    auto layout = calculate_layout(elf);
    std::vector<u8> data(size_for_layout(layout), 0);
    write(data, layout);
    write("ehdr_shdr_manual32.elf", data);
    ELFIO::elfio elfio;
    std::istringstream in(std::string((const char *)data.data(), data.size()));
    CHECK_NOTHROW(elfio.load(in) == true);
    CHECK(elfio.get_class() == ELFIO::ELFCLASS32);
    CHECK(elfio.sections.size() == 2);
    CHECK(elfio.sections[1]->get_name() == ".shstrtab");
  }
  SECTION("Create ehdr, shdr table, and phdr table") {
    Packed elf(ElfFileType::ET_EXEC, ElfMachineType::EM_PEP8, ElfABI::ELFOSABI_NONE);
    ensure_section_header_table(elf);
    Packed::Phdr phdr;
    phdr.p_filesz = 0;
    phdr.p_memsz = 0x1000;
    phdr.p_flags = to_underlying(SegmentFlags::PF_R) | to_underlying(SegmentFlags::PF_W);
    phdr.p_type = to_underlying(SegmentType::PT_LOAD);
    phdr.p_align = 1;
    phdr.p_paddr = phdr.p_vaddr = 0xFEEDBEEF;
    elf.add_segment(std::move(phdr));
    auto layout = calculate_layout(elf);
    std::vector<u8> data(size_for_layout(layout), 0);
    write(data, layout);
    write("ehdr_all_hdr32.elf", data);
    ELFIO::elfio elfio;
    std::istringstream in(std::string((const char *)data.data(), data.size()));
    CHECK_NOTHROW(elfio.load(in) == true);
    CHECK(elfio.get_class() == ELFIO::ELFCLASS32);
    CHECK(elfio.sections.size() == 2);
    CHECK(elfio.segments.size() == 1);
    CHECK(elfio.segments[0]->get_file_size() == 0);
    CHECK(elfio.segments[0]->get_memory_size() == 4096);
    CHECK(elfio.segments[0]->get_sections_num() == 0);
  }
  SECTION("Arrange a symbol table") {
    Packed elf(ElfFileType::ET_EXEC, ElfMachineType::EM_PEP8, ElfABI::ELFOSABI_NONE);
    ensure_section_header_table(elf);
    auto strtab_idx = add_named_section(elf, ".strtab", SectionTypes::SHT_STRTAB);
    auto symtab_idx = add_named_symtab(elf, ".symtab", strtab_idx);
    auto &symtab_data = elf.section_data[symtab_idx];
    PackedSymbolWriter<ElfBits::b32, ElfEndian::le> writer(elf, symtab_idx);
    CHECK(writer.symbol_count() == 0);
    CHECK(symtab_data.size() == 0);

    Packed::Symbol sym1;
    sym1.st_size = 5;
    sym1.st_shndx = 1;
    sym1.st_value = 0x1000;
    sym1.set_bind(SymbolBinding::STB_WEAK);
    writer.add_symbol(std::move(sym1), "alpha");

    Packed::Symbol sym2;
    sym2.st_size = 1;
    sym2.st_shndx = 1;
    sym2.st_value = 0x2000;
    sym2.set_bind(SymbolBinding::STB_LOCAL);
    writer.add_symbol(std::move(sym2), "bravo");

    Packed::Symbol sym3;
    sym3.st_size = 2;
    sym3.st_shndx = 1;
    sym3.st_value = 0x3000;
    sym3.set_bind(SymbolBinding::STB_LOCAL);
    writer.add_symbol(std::move(sym3), "charlie");

    Packed::Symbol sym4;
    sym4.st_size = 4;
    sym4.st_shndx = 1;
    sym4.st_value = 0x4000;
    sym4.set_bind(SymbolBinding::STB_GLOBAL);
    writer.add_symbol(std::move(sym4), "delta");

    Packed::Symbol sym5;
    sym5.st_size = 3;
    sym5.st_shndx = 1;
    sym5.st_value = 0x5000;
    sym5.set_bind(SymbolBinding::STB_LOCAL);
    writer.add_symbol(std::move(sym5), "echo");

    Packed::Symbol sym6;
    sym6.st_size = 6;
    sym6.st_shndx = 1;
    sym6.st_value = 0x6000;
    sym6.set_bind(SymbolBinding::STB_GLOBAL);
    writer.add_symbol(std::move(sym6), "foxtrot");

    // Added 6 symbols + null symbol
    CHECK(writer.symbol_count() == 7);
    writer.arrange_local_symbols();
    // Size field holds desired order.
    for (u32 it = 0; it < writer.symbol_count(); ++it) {
      CHECK(it == writer.get_symbol(it).st_size);
    }
    CHECK(writer.find_symbol(u32{0x2000}) != 0);
    CHECK(writer.find_symbol(u32{0x2001}) == 0);
    CHECK(writer.find_symbol("echo") != 0);
    CHECK(writer.find_symbol("Echo") == 0);

    auto layout = calculate_layout(elf);
    std::vector<u8> data(size_for_layout(layout), 0);
    write(data, layout);
    write("ehdr_symtab32.elf", data);
    ELFIO::elfio elfio;
    std::istringstream in(std::string((const char *)data.data(), data.size()));
    CHECK_NOTHROW(elfio.load(in) == true);
    CHECK(elfio.get_class() == ELFIO::ELFCLASS32);
    CHECK(elfio.sections.size() == 4);
    ELFIO::symbol_section_accessor symbols(elfio, elfio.sections[".symtab"]);
    CHECK(symbols.get_symbols_num() == 7);
  }
  SECTION("Test relocations") {
    // A mirror of "Pepp ASM codegen ELF" test
    Packed elf(ElfFileType::ET_EXEC, ElfMachineType::EM_PEP8, ElfABI::ELFOSABI_NONE);
    ensure_section_header_table(elf);
    auto strtab_idx = add_named_section(elf, ".strtab", SectionTypes::SHT_STRTAB);
    auto symtab_idx = add_named_symtab(elf, ".symtab", strtab_idx);
    auto rel_idx = add_named_rel(elf, ".rel.text", symtab_idx, 1);
    auto &symtab_data = elf.section_data[symtab_idx];
    PackedSymbolWriter<ElfBits::b32, ElfEndian::le> st_writer(elf, symtab_idx);
    Packed::Symbol sym1;
    sym1.st_size = 2;
    sym1.set_type(SymbolType::STT_OBJECT);
    sym1.st_shndx = 1;
    sym1.st_value = 0x2000;
    sym1.set_bind(SymbolBinding::STB_GLOBAL);
    st_writer.add_symbol(std::move(sym1), "a");

    Packed::Symbol sym2;
    sym2.st_size = 0;
    sym2.st_shndx = 0;
    sym2.st_value = 0;
    sym2.set_bind(SymbolBinding::STB_LOCAL);
    st_writer.add_symbol(std::move(sym2), "i");

    Packed::Symbol sym3;
    sym3.st_size = 0;
    sym3.st_shndx = 0;
    sym3.st_value = 0;
    sym3.set_bind(SymbolBinding::STB_LOCAL);
    st_writer.add_symbol(std::move(sym3), "d");

    PackedRelocationWriter<ElfBits::b32, ElfEndian::le> r_writer(elf, rel_idx);
    r_writer.add_rel(6, 0, "d");
    r_writer.add_rel(2, 0, "i");
    r_writer.add_rel(5, 0, "i");

    // Added 3 symbols + null symbol
    CHECK(st_writer.symbol_count() == 4);
    st_writer.arrange_local_symbols([&r_writer](auto a, auto b) { r_writer.swap_symbols(a, b); });

    auto layout = calculate_layout(elf);
    std::vector<u8> data(size_for_layout(layout), 0);
    write(data, layout);
    write("ehdr_rel32.elf", data);
    ELFIO::elfio elfio;
    std::istringstream in(std::string((const char *)data.data(), data.size()));
    CHECK_NOTHROW(elfio.load(in) == true);
    CHECK(elfio.get_class() == ELFIO::ELFCLASS32);
    CHECK(elfio.sections.size() == 5);
    ELFIO::symbol_section_accessor symbols(elfio, elfio.sections[".symtab"]);
    CHECK(symbols.get_symbols_num() == 4);

    auto symtab_ac = ELFIO::symbol_section_accessor(elfio, elfio.sections[".symtab"]);
    auto rel_text_ac = ELFIO::relocation_section_accessor(elfio, elfio.sections[".rel.text"]);
    ELFIO::Elf64_Addr rel_offset;
    ELFIO::Elf_Word rel_symbol;
    unsigned rel_type;
    ELFIO::Elf_Sxword unused;
    std::string sym_name;
    ELFIO::Elf64_Addr sym_value;
    ELFIO::Elf_Xword sym_size;
    unsigned char sym_bind, sym_type, sym_other;
    ELFIO::Elf_Half section_index;
    CHECK(rel_text_ac.get_entries_num() == 3);
    // Entry 0
    rel_text_ac.get_entry(0, rel_offset, rel_symbol, rel_type, unused);
    CHECK(rel_offset == 6);
    CHECK(symtab_ac.get_symbol((ELFIO::Elf_Xword)rel_symbol, sym_name, sym_value, sym_size, sym_bind, sym_type,
                               section_index, sym_other));
    CHECK(sym_name == "d");
    // Entry 1
    rel_text_ac.get_entry(1, rel_offset, rel_symbol, rel_type, unused);
    CHECK(rel_offset == 2);
    CHECK(symtab_ac.get_symbol((ELFIO::Elf_Xword)rel_symbol, sym_name, sym_value, sym_size, sym_bind, sym_type,
                               section_index, sym_other));
    CHECK(sym_name == "i");
    // Entry 2
    rel_text_ac.get_entry(2, rel_offset, rel_symbol, rel_type, unused);
    CHECK(rel_offset == 5);
    CHECK(symtab_ac.get_symbol((ELFIO::Elf_Xword)rel_symbol, sym_name, sym_value, sym_size, sym_bind, sym_type,
                               section_index, sym_other));
    CHECK(sym_name == "i");
  }
  SECTION("Write pepp.mmios notes") {
    Packed elf(ElfFileType::ET_EXEC, ElfMachineType::EM_PEP8, ElfABI::ELFOSABI_NONE);
    ensure_section_header_table(elf);
    auto note_idx = add_named_section(elf, ".note", SectionTypes::SHT_NOTE);
    PackedNoteWriter<ElfBits::b32, ElfEndian::le> note_writer(elf, note_idx);
    char note_data[6]{0x00, 0x07, 0x00, 0x00, 0x01, 0x13};
    note_writer.add_note(std::span<const char>{"pepp.mmios"}, std::span<const char>{note_data, 6}, 0x12);
    note_data[5] = 0x16;
    note_writer.add_note(std::span<const char>{"pepp.mmios"}, std::span<const char>{note_data, 6}, 0x11);
    note_data[5] = 0x1a;
    note_writer.add_note(std::span<const char>{"pepp.mmios"}, std::span<const char>{note_data, 6}, 0x12);
    note_data[5] = 0x19;
    note_writer.add_note(std::span<const char>{"pepp.mmios"}, std::span<const char>{note_data, 6}, 0x13);
    auto layout = calculate_layout(elf);
    std::vector<u8> data(size_for_layout(layout), 0);
    write(data, layout);
    write("ehdr_notes.elf", data);
  }
  SECTION("Write dynamic tags") {
    using enum DynamicTags;
    Packed elf(ElfFileType::ET_EXEC, ElfMachineType::EM_386, ElfABI::ELFOSABI_NONE);
    ensure_section_header_table(elf);
    elf.add_segment(SegmentType::PT_DYNAMIC); // dynamic

    std::deque<AbsoluteFixup> fixups;
    auto symtab_idx = add_named_symtab(elf, ".symtab", add_named_section(elf, ".strtab", SectionTypes::SHT_STRTAB));
    auto dynstr_idx = add_named_section(elf, ".dynstr", SectionTypes::SHT_STRTAB);
    auto dynamic_idx = add_named_dynamic(elf, ".dynamic", dynstr_idx);

    {
      PackedSymbolWriter<ElfBits::b32, ElfEndian::le> st_writer(elf, symtab_idx);
      auto _DYNAMIC = st_writer.add_symbol(create_null_symbol<ElfBits::b32, ElfEndian::le>(), "_DYNAMIC", dynamic_idx);
      fixups.emplace_back(st_writer.fixup_value(_DYNAMIC, std::function<word<ElfBits::b32>()>{[&]() {
                                                  return word<ElfBits::b32>{elf.section_headers[dynamic_idx].sh_offset};
                                                }}));
      st_writer.arrange_local_symbols();
    }
    {
      PackedDynamicWriter<ElfBits::b32, ElfEndian::le> dyn_writer(elf, dynamic_idx);
      fixups.emplace_back(dyn_writer.fixup_value(dyn_writer.add_entry(DT_INIT), []() { return 0xFEED; }));
      fixups.emplace_back(dyn_writer.fixup_value(dyn_writer.add_entry(DT_FINI), []() { return 0xBEEF; }));
      dyn_writer.add_entry(DT_NULL);
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
    write("ehdr_dynamic.elf", data);
  }
  SECTION("Write .init") {
    Packed elf(ElfFileType::ET_EXEC, ElfMachineType::EM_386, ElfABI::ELFOSABI_NONE);
    ensure_section_header_table(elf);
    auto idx = add_named_section(elf, ".init", SectionTypes::SHT_INIT_ARRAY);
    PackedArrayWriter<ElfBits::b32, ElfEndian::le> writer(elf, idx);
    writer.add_entry(0xFEED);
    writer.add_entry(0xBEEF0000);
    auto layout = calculate_layout(elf);
    std::vector<u8> data(size_for_layout(layout), 0);
    write(data, layout);
    write("ehdr_array.elf", data);
  }
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
  SECTION("Emit a working .hash section") {
    using enum DynamicTags;

    Packed elf(ElfFileType::ET_EXEC, ElfMachineType::EM_386, ElfABI::ELFOSABI_NONE);
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
      PackedHashedSymbolWriter<ElfBits::b32, ElfEndian::le> hs_writer(elf, hash_idx);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "alpha", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "bravo", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "charlie", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "delta", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "echo", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "foxtrot", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "golf", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "hotel", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "india", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "juliett", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "kilo", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "lima", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "mike", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "november", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "oscar", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "papa", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "quebec", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "romeo", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "sierra", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "tango", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "uniform", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "victor", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "whiskey", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "x-ray", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "yankee", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "zulu", 1);
      hs_writer.arrange_local_symbols();
      hs_writer.compute_hash_table(13);
    }
    {
      PackedSymbolWriter<ElfBits::b32, ElfEndian::le> st_writer(elf, symtab_idx);
      auto _DYNAMIC = st_writer.add_symbol(create_null_symbol<ElfBits::b32, ElfEndian::le>(), "_DYNAMIC", dynamic_idx);
      st_writer.arrange_local_symbols();
      fixups.emplace_back(st_writer.fixup_value(_DYNAMIC, [&]() { return elf.program_headers[0].p_paddr; }));
    }
    {
      PackedDynamicWriter<ElfBits::b32, ElfEndian::le> dyn_writer(elf, dynamic_idx);
      fixups.emplace_back(dyn_writer.fixup_value(dyn_writer.add_entry(DT_STRTAB),
                                                 [&]() { return elf.section_headers[dynstr_idx].sh_addr; }));
      fixups.emplace_back(dyn_writer.fixup_value(dyn_writer.add_entry(DT_STRSZ),
                                                 [&]() { return elf.section_headers[dynstr_idx].sh_size; }));
      fixups.emplace_back(dyn_writer.fixup_value(dyn_writer.add_entry(DT_SYMTAB),
                                                 [&]() { return elf.section_headers[dynsym_idx].sh_addr; }));
      fixups.emplace_back(dyn_writer.fixup_value(dyn_writer.add_entry(DT_HASH),
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
    write("ehdr_hash.elf", data);

    // Easiest way to verify manually is download llvm for llvm-readelf and run:
    // llvm-readelf-15 --gnu-hash-table -a --hash-symbols -d --dyn-syms ehdr_hash.elf
    PackedHashedSymbolReader<ElfBits::b32, ElfEndian::le> hs_reader(elf, hash_idx);
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
  SECTION("Emit a working .gnu.hash section") {
    using enum DynamicTags;

    Packed elf(ElfFileType::ET_EXEC, ElfMachineType::EM_386, ElfABI::ELFOSABI_NONE);
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
      PackedGNUHashedSymbolWriter<ElfBits::b32, ElfEndian::le> hs_writer(elf, hash_idx);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "alpha", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "bravo", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "charlie", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "delta", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "echo", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "foxtrot", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "golf", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "hotel", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "india", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "juliett", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "kilo", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "lima", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "mike", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "november", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "oscar", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "papa", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "quebec", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "romeo", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "sierra", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "tango", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "uniform", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "victor", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "whiskey", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "x-ray", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "yankee", 1);
      hs_writer.add_symbol(create_global_symbol<ElfBits::b32, ElfEndian::le>(), "zulu", 1);
      hs_writer.arrange_local_symbols();
      hs_writer.compute_hash_table(11, 4, 2, 5);
    }
    {
      PackedSymbolWriter<ElfBits::b32, ElfEndian::le> st_writer(elf, symtab_idx);
      auto _DYNAMIC = st_writer.add_symbol(create_null_symbol<ElfBits::b32, ElfEndian::le>(), "_DYNAMIC", dynamic_idx);
      st_writer.arrange_local_symbols();
      fixups.emplace_back(st_writer.fixup_value(_DYNAMIC, [&]() { return elf.program_headers[0].p_paddr; }));
    }
    {
      PackedDynamicWriter<ElfBits::b32, ElfEndian::le> dyn_writer(elf, dynamic_idx);
      fixups.emplace_back(dyn_writer.fixup_value(dyn_writer.add_entry(DT_STRTAB),
                                                 [&]() { return elf.section_headers[dynstr_idx].sh_addr; }));
      fixups.emplace_back(dyn_writer.fixup_value(dyn_writer.add_entry(DT_STRSZ),
                                                 [&]() { return elf.section_headers[dynstr_idx].sh_size; }));
      fixups.emplace_back(dyn_writer.fixup_value(dyn_writer.add_entry(DT_SYMTAB),
                                                 [&]() { return elf.section_headers[dynsym_idx].sh_addr; }));
      fixups.emplace_back(dyn_writer.fixup_value(dyn_writer.add_entry(DT_GNU_HASH),
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
    write("ehdr_gnuhash.elf", data);

    // Assuming nbuckets=11, symndx=mask_words=11, shift2=5, symndx=4 (start hash at delta)
    // Easiest way to verify manually is download llvm for llvm-readelf and run:
    // llvm-readelf-15 --gnu-hash-table -a --hash-symbols -d --dyn-syms ehdr_gnuhash.elf
    PackedGNUHashedSymbolReader<ElfBits::b32, ElfEndian::le> hs_reader(elf, hash_idx);
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
}

TEST_CASE("Test custom ELF library, 64-bit", "[scope:elf][kind:unit][arch:*]") {
  using namespace pepp::bts;
  using Packed = PackedElfLE64;
  SECTION("Create ehdr with custom, read with ELFIO") {
    auto my_header = Packed::Ehdr(ElfFileType::ET_EXEC, ElfMachineType::EM_RISCV, ElfABI::ELFOSABI_NONE);
    auto data = bits::span<const u8>{reinterpret_cast<const u8 *>(&my_header), sizeof(my_header)};
    CHECK(data.size() == 64);
    write("ehdr_only_test64.elf", data);
    ELFIO::elfio elf;
    std::istringstream in(std::string((const char *)data.data(), data.size()));
    CHECK_NOTHROW(elf.load(in) == true);
    CHECK(elf.get_class() == ELFIO::ELFCLASS64);
  }
}
