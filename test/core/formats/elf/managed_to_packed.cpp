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

#include "core/formats/elf/managed_to_packed.hpp"
#include <catch.hpp>
#include <elfio/elfio.hpp>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>
#include "core/compile/symbol/entry.hpp"
#include "core/compile/symbol/leaf_table.hpp"
#include "core/compile/symbol/value.hpp"
#include "core/formats/elf/managed_elf.hpp"
#include "core/formats/elf/managed_section.hpp"
#include "core/formats/elf/managed_section_gc.hpp"
#include "core/formats/elf/managed_section_shstrtab.hpp"
#include "core/formats/elf/managed_section_strtab.hpp"
#include "core/formats/elf/managed_section_symtab.hpp"
#include "core/formats/elf/packed_elf.hpp"
#include "core/formats/elf/packed_ops.hpp"

namespace {
using namespace pepp::bts;
using Packed = PackedGrowableElfBE32;
using LeafTable = pepp::core::symbol::LeafTable;

// An allocated PROGBITS section carrying `data`.
SectionRef add(ManagedElf &elf, const char *name, std::vector<u8> data, uxword addr = 0, u32 addralign = 1) {
  auto ref = elf.add_section(name, SectionTypes::SHT_PROGBITS);
  auto *sec = elf.section(ref);
  sec->addr = addr, sec->addralign = addralign, sec->flags = SectionFlags::SHF_ALLOC;
  sec->content.emplace<RawBytes>().bytes = std::move(data);
  return ref;
}

ELFIO::elfio read(const std::vector<u8> &bytes) {
  std::istringstream in(std::string(reinterpret_cast<const char *>(bytes.data()), bytes.size()));
  ELFIO::elfio elf;
  REQUIRE(elf.load(in));
  return elf;
}

// Contains both the PackedElf and the bytes it serialized to.
struct Serialized {
  PackResult result;
  std::vector<u8> bytes;
  Packed &packed() { return *std::get<std::unique_ptr<Packed>>(result.elf); }
  u16 index_of(SectionRef ref) const { return result.section_indices.at(ref); }
  const Packed::Shdr &shdr(SectionRef ref) { return packed().section_headers[index_of(ref)]; }
  const AStorage &data(SectionRef ref) { return *packed().section_data[index_of(ref)]; }
};

// Do not include any segment layout constraints, which is true for ET_REL files.
Serialized serialize(ManagedElf &elf, std::vector<SectionRef> live) {
  Serialized ret{pack(elf, live), {}};
  auto layout = calculate_layout(ret.packed());
  ret.bytes.resize(size_for_layout(layout), 0);
  write(ret.bytes, layout);
  return ret;
}

// Keep all sections and insert a section header string table.
Serialized serialize(ManagedElf &elf) {
  auto live = garbage_collect_sections(elf);
  build_shstrtab(elf, live);
  return serialize(elf, std::move(live));
}
} // namespace

TEST_CASE("Convert ManagedElf to PackedElf", "[kind:unit][arch:*][tc2][scope:elf]") {
  ManagedElf elf(ElfBits::b32, ElfEndian::be, ElfFileType::ET_REL, ElfMachineType::EM_PEP10);

  SECTION("File header fields are copied") {
    elf.abi_version = 3, elf.flags = 0xF00D, elf.entry = 0x1234;
    add(elf, ".text", {1, 2, 3, 4});
    auto reader = read(serialize(elf).bytes);
    CHECK(reader.get_class() == ELFIO::ELFCLASS32);
    CHECK(reader.get_encoding() == ELFIO::ELFDATA2MSB);
    CHECK(reader.get_type() == ELFIO::ET_REL);
    CHECK(reader.get_machine() == bits::to_underlying(ElfMachineType::EM_PEP10));
    CHECK(reader.get_abi_version() == 3);
    CHECK(reader.get_flags() == 0xF00D);
    CHECK(reader.get_entry() == 0x1234);
  }

  SECTION("Section index 0 is SHN_UNDEF; sections sorted by ascending address") {
    auto data = add(elf, ".data", {9, 9}, 0x2000);
    auto text = add(elf, ".text", {1, 2, 3}, 0x1000);
    auto out = serialize(elf);
    auto reader = read(out.bytes);
    // 2 above plus null and shstrtab
    REQUIRE(out.packed().section_headers.size() == 4);
    CHECK(out.packed().section_headers[0].sh_type == bits::to_underlying(SectionTypes::SHT_NULL));
    // .data was added first but sits higher in memory, so .text is written first.
    CHECK(out.index_of(text) == 1);
    CHECK(out.index_of(data) == 2);
    CHECK(out.shdr(text).sh_addr == 0x1000);
    CHECK(out.shdr(data).sh_addr == 0x2000);
    CHECK(reader.sections[1]->get_name() == ".text");
    CHECK(reader.sections[1]->get_address() == 0x1000);
    CHECK(reader.sections[2]->get_name() == ".data");
    CHECK(reader.sections[2]->get_address() == 0x2000);
  }

  SECTION("Unaddressed sections are sorted by insertion order") {
    add(elf, ".text", {1});
    add(elf, ".data", {2});
    add(elf, ".rodata", {3});
    auto reader = read(serialize(elf).bytes);
    CHECK(reader.sections[1]->get_name() == ".text");
    CHECK(reader.sections[2]->get_name() == ".data");
    CHECK(reader.sections[3]->get_name() == ".rodata");
    CHECK(reader.sections[4]->get_name() == ".shstrtab");
  }

  SECTION("Content is copied and respects alignment") {
    auto text = add(elf, ".text", {1, 2, 3, 4}, 0, 2);
    auto out = serialize(elf);
    auto reader = read(out.bytes);
    REQUIRE(out.shdr(text).sh_size == 4);
    REQUIRE(out.data(text).size() == 4);
    const auto stored = out.data(text).get(0, 4);
    CHECK(std::vector<u8>(stored.begin(), stored.end()) == std::vector<u8>{1, 2, 3, 4});
    const auto *read_text = reader.sections[".text"];
    REQUIRE(read_text != nullptr);
    CHECK(read_text->get_type() == ELFIO::SHT_PROGBITS);
    CHECK(read_text->get_flags() == ELFIO::SHF_ALLOC);
    CHECK(read_text->get_addr_align() == 2);
    REQUIRE(read_text->get_size() == 4);
    CHECK(std::vector<u8>(read_text->get_data(), read_text->get_data() + 4) == std::vector<u8>{1, 2, 3, 4});
    CHECK(read_text->get_offset() % read_text->get_addr_align() == 0);
  }

  SECTION("NOBITS has a (memory) size but occupies no file bytes") {
    add(elf, ".text", {1, 2, 3, 4});
    auto bss = elf.add_section(".bss", SectionTypes::SHT_NOBITS);
    elf.section(bss)->content.emplace<NoBits>().size = 0x1000;
    auto out = serialize(elf);
    auto reader = read(out.bytes);
    CHECK(out.shdr(bss).sh_size == 0x1000);
    CHECK(out.data(bss).size() == 0); // nothing in the file
    const auto *read_bss = reader.sections[".bss"];
    REQUIRE(read_bss != nullptr);
    CHECK(read_bss->get_type() == ELFIO::SHT_NOBITS);
    // sh_size is the memory size and is much larger than the overall file size.
    CHECK(read_bss->get_size() == 0x1000);
    CHECK(out.bytes.size() < 0x1000);
  }

  SECTION("sh_name resolves via .shstrtab") {
    auto text = add(elf, ".text", {1});
    add(elf, ".data", {2});
    auto out = serialize(elf);
    auto reader = read(out.bytes);
    const auto &names = *elf.section(elf.shstrtab())->content_as<ManagedStringTable>();
    CHECK(out.shdr(text).sh_name == names.offset_of(*names.find(".text")));
    CHECK(out.packed().header.e_shstrndx == out.index_of(elf.shstrtab()));
    for (const char *name : {".text", ".data", ".shstrtab"}) {
      INFO(name);
      CHECK(reader.sections[name] != nullptr);
    }
    REQUIRE(reader.get_section_name_str_index() != ELFIO::SHN_UNDEF);
    CHECK(reader.sections[reader.get_section_name_str_index()]->get_name() == ".shstrtab");
  }

  SECTION("SectionRef maps to indices and accounts for reserved values") {
    auto text = add(elf, ".text", {1});
    auto data = add(elf, ".data", {2});
    auto strtab = elf.add_section(".strtab", SectionTypes::SHT_STRTAB);
    auto symtab = elf.add_section(".symtab", SectionTypes::SHT_SYMTAB);
    auto rel = elf.add_section(".rel.text", SectionTypes::SHT_REL);
    elf.section(symtab)->link = strtab;
    // A count rather than a section, which is what sh_info means for a symbol table.
    elf.section(symtab)->info = u32{3};
    elf.section(rel)->link = symtab, elf.section(rel)->info = text;
    elf.section(text)->link = ManagedElf::SHN_ABS;
    elf.section(data)->link = ManagedElf::SHN_COMMON;
    auto out = serialize(elf);
    auto reader = read(out.bytes);
    CHECK(out.shdr(symtab).sh_link == out.index_of(strtab));
    CHECK(out.shdr(symtab).sh_info == 3); // a count, not a section
    CHECK(out.shdr(rel).sh_info == out.index_of(text));
    CHECK(out.shdr(text).sh_link == static_cast<u32>(SectionIndices::SHN_ABS));
    CHECK(out.shdr(data).sh_link == static_cast<u32>(SectionIndices::SHN_COMMON));
    const auto *read_symtab = reader.sections[".symtab"];
    const auto *read_rel = reader.sections[".rel.text"];
    REQUIRE(read_symtab != nullptr);
    REQUIRE(read_rel != nullptr);
    CHECK(read_symtab->get_link() == reader.sections[".strtab"]->get_index());
    CHECK(read_rel->get_link() == read_symtab->get_index());
    CHECK(read_rel->get_info() == reader.sections[".text"]->get_index());
  }

  SECTION("sh_entsize set for sections with fixed-size entries") {
    auto text = add(elf, ".text", {1});
    auto symtab = elf.add_section(".symtab", SectionTypes::SHT_SYMTAB);
    auto rela = elf.add_section(".rela.text", SectionTypes::SHT_RELA);
    auto out = serialize(elf);
    CHECK(out.shdr(text).sh_entsize == 0);
    // Per ELF TIS figures 1-15 and 1-20, for a 32-bit file.
    CHECK(out.shdr(symtab).sh_entsize == 16);
    CHECK(out.shdr(rela).sh_entsize == 12);
  }

  SECTION("A file with no .shstrtab has no section names") {
    add(elf, ".text", {1});
    add(elf, ".data", {2});
    // When e_shstrndx is SHN_UNDEF then every name is then 0.
    auto out = serialize(elf, garbage_collect_sections(elf));
    auto reader = read(out.bytes);
    REQUIRE(out.packed().section_headers.size() == 3);
    CHECK(out.packed().header.e_shstrndx == 0);
    for (const auto &shdr : out.packed().section_headers) CHECK(shdr.sh_name == 0);
    CHECK(reader.get_section_name_str_index() == ELFIO::SHN_UNDEF);
    REQUIRE(reader.sections.size() == 3);
    for (const auto &sec : reader.sections) CHECK(sec->get_name().empty());
  }

  SECTION("A symbol table") {
    using namespace pepp::core::symbol;
    auto text = add(elf, ".text", {1, 2, 3, 4});
    auto symbols = std::make_shared<LeafTable>(2);
    // One local code symbol, one global, one .EQUATE-like constant, one external the linker must
    // supply, and one that is garbage collected. Should exercise all meaningful code paths.
    auto helper = symbols->define("helper");
    helper->value = std::make_shared<LocationValue>(1, 2, 0, 2, Type::Code);
    auto entry_point = symbols->define("entry");
    entry_point->binding = Binding::Global;
    entry_point->visibility = Visibility::Protected;
    entry_point->value = std::make_shared<LocationValue>(1, 2, 0, 0, Type::Code);
    auto limit = symbols->define("limit");
    limit->value = std::make_shared<ConstantValue>(bits::MaskedBits{2, 0xBEEF, 0xFFFF});
    auto external = symbols->reference("charin");
    external->binding = Binding::Global;
    auto unused = symbols->define("unused");

    auto symtab_ref = elf.add_section(".symtab", SectionTypes::SHT_SYMTAB);
    auto &table = elf.section(symtab_ref)->make_content<ManagedSymbolTable>(symbols);
    for (const auto &defined : {helper, entry_point}) table.set_section(defined, text);
    freeze_symbols(elf, symtab_ref, [&](const auto &e) { return e != unused; });

    auto live = garbage_collect_sections(elf);
    build_strtabs_for_symtabs(elf, live);
    build_shstrtab(elf, live);
    auto out = serialize(elf, live);
    auto reader = read(out.bytes);

    const auto *read_symtab = reader.sections[".symtab"];
    REQUIRE(read_symtab != nullptr);
    CHECK(read_symtab->get_link() ==
          reader.sections[".strtab"]->get_index()); // sh_link names the string table the pass created
    CHECK(read_symtab->get_info() == 3);            // first global is 4th symbol (1 null, 2 locals)
    CHECK(read_symtab->get_entry_size() == 16);     // ELFIO agrees on our entry size
    CHECK(read_symtab->get_addr_align() == 4);

    ELFIO::symbol_section_accessor read_symbols(reader, const_cast<ELFIO::section *>(read_symtab));
    // The null symbol, two locals, and two globals.
    REQUIRE(read_symbols.get_symbols_num() == 5);

    // Helper struct to make it easier to work with ELFIOs 8 out param functions.
    struct Symbol {
      std::string name;
      ELFIO::Elf64_Addr value;
      ELFIO::Elf_Xword size;
      unsigned char bind, type;
      ELFIO::Elf_Half section;
      unsigned char other;
    };
    auto at = [&](unsigned index) {
      Symbol sym;
      REQUIRE(read_symbols.get_symbol(index, sym.name, sym.value, sym.size, sym.bind, sym.type, sym.section,
                                      sym.other));
      return sym;
    };
    const auto in_text = reader.sections[".text"]->get_index();

    // Locals first, ordered by name, so .strtab offsets and symbol indices agree on one order.
    const auto s_null = at(0), s_helper = at(1), s_limit = at(2), s_charin = at(3), s_entry = at(4);

    // Index 0 is the reserved null symbol, and nothing may have been written over it.
    CHECK(s_null.name.empty());
    CHECK(s_null.value == 0);
    CHECK(s_null.section == ELFIO::SHN_UNDEF);
    CHECK(s_null.type == ELFIO::STT_NOTYPE);

    CHECK(s_helper.name == "helper");
    CHECK(s_helper.bind == ELFIO::STB_LOCAL);
    CHECK(s_helper.type == ELFIO::STT_FUNC);
    CHECK(s_helper.value == 2);
    CHECK(s_helper.section == in_text);

    // A constant belongs to no section
    CHECK(s_limit.name == "limit");
    CHECK(s_limit.section == ELFIO::SHN_ABS);
    CHECK(s_limit.value == 0xBEEF);

    // An undefined external keeps its binding
    CHECK(s_charin.name == "charin");
    CHECK(s_charin.bind == ELFIO::STB_GLOBAL);
    CHECK(s_charin.section == ELFIO::SHN_UNDEF);

    CHECK(s_entry.name == "entry");
    CHECK(s_entry.bind == ELFIO::STB_GLOBAL);
    CHECK(s_entry.other == ELFIO::STV_PROTECTED);
    CHECK(s_entry.section == in_text);

    // Garbage collected symbol was not written.
    for (unsigned it = 0; it < read_symbols.get_symbols_num(); ++it) CHECK(at(it).name != "unused");
  }

  SECTION("Garbage-collected sections are not serialized") {
    add(elf, ".keep", {1});
    add(elf, ".drop", {2});
    auto live = garbage_collect_sections(elf, [](const ManagedSection &sec) { return sec.name != ".drop"; });
    build_shstrtab(elf, live);
    auto reader = read(serialize(elf, std::move(live)).bytes);
    CHECK(reader.sections[".keep"] != nullptr);
    CHECK(reader.sections[".drop"] == nullptr);
    // Its name is not in .shstrtab either, since build_shstrtab only sees the live set.
    CHECK(reader.sections.size() == 3);
  }
}

TEST_CASE("Convert ManagedElf to PackedElf failures", "[kind:unit][arch:*][!throws][tc2][scope:elf]") {
  ManagedElf elf(ElfBits::b32, ElfEndian::be, ElfFileType::ET_REL, ElfMachineType::EM_PEP10);

  SECTION("Do not truncate wide values") {
    add(elf, ".text", {1}, 0x1'0000'0000ull);
    auto live = garbage_collect_sections(elf);
    build_shstrtab(elf, live);
    CHECK_THROWS_AS(pack(elf, live), std::logic_error);
  }

  SECTION(".shstrta must contain all sections' names") {
    add(elf, ".text", {1});
    auto live = garbage_collect_sections(elf);
    build_shstrtab(elf, live);
    live.push_back(add(elf, ".late", {2}));
    CHECK_THROWS_AS(pack(elf, live), std::logic_error);
  }
}

TEST_CASE("Convert ManagedElf to PackedElf bitness/endianness checks", "[kind:unit][arch:*][tc2][scope:elf]") {
  struct Built {
    std::unique_ptr<ManagedElf> elf;
    std::vector<SectionRef> live;
    SectionRef text, symtab;
  };
  auto build = [](ElfBits bits, ElfEndian endian) {
    auto elf = std::make_unique<ManagedElf>(bits, endian, ElfFileType::ET_REL, ElfMachineType::EM_PEP10);
    auto text = add(*elf, ".text", {1, 2, 3, 4});
    auto symtab = elf->add_section(".symtab", SectionTypes::SHT_SYMTAB);
    auto live = garbage_collect_sections(*elf);
    build_shstrtab(*elf, live);
    return Built{std::move(elf), std::move(live), text, symtab};
  };

  SECTION("Test e_ident for each bitness/endianness") {
    auto check = [&](const char *what, ElfBits bits, ElfEndian endian, ElfClass _class, ElfEncoding encoding) {
      INFO(what);
      auto built = build(bits, endian);
      const auto inner = [&](const auto &packed) {
        using Chosen = std::remove_cvref_t<decltype(*packed)>;
        CHECK(Chosen::elf_bits == bits);
        CHECK(Chosen::elf_endian == endian);
        CHECK(packed->header.has_valid_magic());
        CHECK(packed->header.ei_class() == _class);
        CHECK(packed->header.ei_data() == encoding);
        CHECK(packed->header.ei_version() == ElfVersion::EV_CURRENT);
      };
      std::visit(inner, pack(*built.elf, built.live).elf);
    };
    check("LE32", ElfBits::b32, ElfEndian::le, ElfClass::ELFCLASS32, ElfEncoding::ELFDATA2LSB);
    check("BE32", ElfBits::b32, ElfEndian::be, ElfClass::ELFCLASS32, ElfEncoding::ELFDATA2MSB);
    check("LE64", ElfBits::b64, ElfEndian::le, ElfClass::ELFCLASS64, ElfEncoding::ELFDATA2LSB);
    check("BE64", ElfBits::b64, ElfEndian::be, ElfClass::ELFCLASS64, ElfEncoding::ELFDATA2MSB);
  }

  SECTION("64-bit ELF fields not narrowed to 32-bits") {
    auto built = build(ElfBits::b64, ElfEndian::le);
    built.elf->section(built.text)->addr = 0x1'0000'0000ull;
    auto result = pack(*built.elf, built.live);
    auto &packed = *std::get<std::unique_ptr<PackedGrowableElfLE64>>(result.elf);
    CHECK(packed.section_headers[result.section_indices.at(built.text)].sh_addr == 0x1'0000'0000ull);
  }
}
