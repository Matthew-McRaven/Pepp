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
} // namespace

TEST_CASE("Test custom ELF library, 32-bit", "[scope:elf][kind:unit][arch:*]") {
  using namespace pepp::bts;
  using Packed = PackedGrowableElfLE32;
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
    CHECK(symtab_data->size() == 0);

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
      fixups.emplace_back(fixup_symbol_value(
          elf, symtab_idx, _DYNAMIC, [&]() { return word<ElfBits::b32>{elf.section_headers[dynamic_idx].sh_offset}; }));
      st_writer.arrange_local_symbols();
    }
    {
      PackedDynamicWriter<ElfBits::b32, ElfEndian::le> dyn_writer(elf, dynamic_idx);
      fixups.emplace_back(
          fixup_dynamic_value(elf, dynamic_idx, dyn_writer.add_entry(DT_INIT), [&]() { return 0xFEED; }));
      fixups.emplace_back(
          fixup_dynamic_value(elf, dynamic_idx, dyn_writer.add_entry(DT_FINI), [&]() { return 0xBEEF; }));
      dyn_writer.add_entry(DT_NULL);
    }

    SegmentLayoutConstraint constraint_dyn;
    constraint_dyn.alignment = 4096;
    constraint_dyn.from_sec = dynstr_idx;
    constraint_dyn.to_sec = dynamic_idx;
    constraint_dyn.base_address = 0xFEED;
    std::vector<SegmentLayoutConstraint> constraints = {constraint_dyn};

    auto layout = calculate_layout(elf, &constraints);
    // for (const auto &fixup : fixups) fixup.update();
    std::vector<u8> data(size_for_layout(layout), 0);
    write(data, layout);
    write("ehdr_dynamic.elf", data);
    // WARNING: exploding test.
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
