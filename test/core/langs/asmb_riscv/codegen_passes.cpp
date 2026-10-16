/*
 * Copyright (c) 2023-2024 J. Stanley Warford, Matthew McRaven
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
#include <algorithm>
#include <map>
#include <sstream>
#include <vector>
#include "core/arch/riscv/asmb/rvi_patterns.hpp"
#include "core/compile/ir_linear/line_empty.hpp"
#include "core/compile/ir_linear/line_symbol.hpp"
#include "core/compile/symbol/entry.hpp"
#include "core/compile/symbol/leaf_table.hpp"
#include "core/langs/asmb/codegen_error.hpp"
#include "core/langs/asmb/debug_line.hpp"
#include "core/langs/asmb/diagnostic_table.hpp"
#include "core/langs/asmb_riscv/codegen.hpp"
#include "core/langs/asmb_riscv/parser.hpp"
#include "elfio/elfio.hpp"

namespace {
static auto data = [](auto str) { return pepp::tc::support::SeekableData{str}; };
} // namespace

TEST_CASE("RISCV ASM code generator",
          "[scope:core][scope:core.langs][level:asmb3][level:asmb5][kind:unit][arch:riscv]") {
  using Lexer = pepp::langs::RISCVLexer;
  using Parser = pepp::tc::parser::RISCVParser;
  using SymbolTable = pepp::core::symbol::LeafTable;
  using namespace pepp::tc;
  SECTION("R Type: add x1, x2, x3") { // matches sources of samples add.S
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("add x1, x2, x3"));
    auto results = p.parse(diag);
    CHECK(diag.count() == 0);
    auto result = pepp::tc::riscv_split_to_sections(diag, results);
    CHECK(diag.count() == 0);
    auto symbol_tab = p.symbol_table();
    auto &sections = result.grouped_ir;
    auto addresses = pepp::tc::riscv_assign_addresses(sections, 0xfeed);
    CHECK(addresses.container.size() == 1);
    auto instr = std::dynamic_pointer_cast<RTypeIR>(sections[0].second[0]);
    CHECK(instr != nullptr);
    CHECK(instr->mnemonic.mn == riscv::ADD);
    CHECK(addresses.count(&*instr) == 1);
    CHECK(addresses.at(&*instr).address == 0xfeed);
    CHECK(addresses.at(&*instr).size == 4);
    auto object_code = pepp::tc::riscv_to_object_code(diag, addresses, sections);
    auto elf_result = pepp::tc::riscv_to_elf(sections, results, addresses, object_code, *symbol_tab);
    // Read what would actually be written, rather than the in-memory model of it.
    const auto bytes = pepp::tc::elf_bytes(elf_result);
    std::istringstream in(std::string(reinterpret_cast<const char *>(bytes.data()), bytes.size()));
    ELFIO::elfio elf;
    REQUIRE(elf.load(in));
    REQUIRE(elf.sections[".text"] != nullptr);
    CHECK(elf.sections[".text"]->get_size() == 4);
    // Per samples directory, bytes should be little-endian b3 00 31 00
    auto text_section = elf.sections[".text"];
    auto data = text_section->get_data();
    CHECK((u8)data[0] == 0xb3);
    CHECK((u8)data[1] == 0x00);
    CHECK((u8)data[2] == 0x31);
    CHECK((u8)data[3] == 0x00);
  }
  SECTION("DWARF 5 .debug_line output") {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data("add x1, x2, x3\nsub x1, x2, x3"));
    auto results = p.parse(diag);
    auto result = pepp::tc::riscv_split_to_sections(diag, results);
    auto &sections = result.grouped_ir;
    auto addresses = pepp::tc::riscv_assign_addresses(sections, 0x1000);
    auto object_code = pepp::tc::riscv_to_object_code(diag, addresses, sections);
    // An empty name keeps its default.
    const pepp::tc::DebugLineFiles files{.source = "prog.s", .listing = "", .directory = "/work"};
    auto elf_result = pepp::tc::riscv_to_elf(sections, results, addresses, object_code, *p.symbol_table(), files);
    REQUIRE(diag.count() == 0);
    const auto bytes = pepp::tc::elf_bytes(elf_result);
    std::istringstream in(std::string(reinterpret_cast<const char *>(bytes.data()), bytes.size()));
    ELFIO::elfio elf;
    REQUIRE(elf.load(in));
    const auto bytes_of = [&](const char *name) {
      const auto *sec = elf.sections[name];
      REQUIRE(sec != nullptr);
      return bits::span<const u8>(reinterpret_cast<const u8 *>(sec->get_data()), static_cast<size_t>(sec->get_size()));
    };
    const auto encoded = bytes_of(pepp::tc::debugLineSection);

    const auto source = pepp::bts::parse_line_program_unit<pepp::bts::ElfEndian::le>(encoded, 0);
    REQUIRE(source.has_value());
    CHECK(source->header.address_size == 4);
    CHECK(source->header.files[0].path == "prog.s");
    CHECK(source->header.directories == std::vector<std::string>{"/work"});
    std::vector<std::pair<u64, u64>> rows;
    const auto ran = pepp::bts::run_line_program<pepp::bts::ElfEndian::le>(
        *source, [&](const auto &row, auto) { rows.emplace_back(row.address, row.end_sequence ? 0 : row.line); });
    REQUIRE(ran.has_value());
    // The sequence ends after the second instruction.
    CHECK(rows == std::vector<std::pair<u64, u64>>{{0x1000, 1}, {0x1004, 2}, {0x1008, 0}});

    CHECK(source->next_unit == encoded.size());

    const auto listing_encoded = bytes_of(pepp::tc::debugListingLineSection);
    const auto listing = pepp::bts::parse_line_program_unit<pepp::bts::ElfEndian::le>(listing_encoded, 0);
    REQUIRE(listing.has_value());
    CHECK(listing->header.files[0].path == pepp::tc::debugLineListingFile);
    CHECK(listing->next_unit == listing_encoded.size());

    // The compile unit is named after the source unit's file
    const auto *info = elf.sections[pepp::tc::debugInfoSection];
    REQUIRE(info != nullptr);
    const auto info_bytes = std::string_view(info->get_data(), info->get_size());
    CHECK(info_bytes.find("prog.s") != std::string_view::npos);
    CHECK(info_bytes.find("/work") != std::string_view::npos);

    const auto input = pepp::tc::to_input_elf(elf_result);
    const auto mappings = std::visit([](const auto &file) { return pepp::tc::read_line_mappings(*file); }, input);
    REQUIRE(mappings.has_value());
    REQUIRE(mappings->size() == 2);
    CHECK((*mappings)[0].source_line == 1);
    CHECK((*mappings)[0].listing_line != 0);
    CHECK((*mappings)[1].source_line == 2);
    CHECK((*mappings)[1].listing_line > (*mappings)[0].listing_line);
  }
  SECTION("Symbols for B-/J-type instructions encode as relative offsets") {
    const auto jump = [](const char *source) {
      pepp::tc::DiagnosticTable diag;
      auto p = Parser(data(source));
      auto results = p.parse(diag);
      auto result = pepp::tc::riscv_split_to_sections(diag, results);
      REQUIRE(diag.count() == 0);
      auto addresses = pepp::tc::riscv_assign_addresses(result.grouped_ir, 0x100);
      const auto code = pepp::tc::riscv_to_object_code(diag, addresses, result.grouped_ir);
      const auto bytes = code.section_slices.at(0).get(4, 4);
      return std::vector<u8>(bytes.begin(), bytes.end());
    };
    // The j at 0x104 targets 0x108, which is 4 bytes ahead.
    CHECK(jump("nop\nj target\ntarget: nop") == jump("nop\nj 4\nnop"));
    // . jumps to self
    CHECK(jump("nop\nj .") == jump("nop\nj 0"));
    CHECK(jump("nop\nl: j . + 4\nnop") == jump("nop\nj 4\nnop"));
  }
}

namespace {
struct Expect {
  const char *name;
  u64 address, size;
  pepp::core::symbol::Type type;
};
} // namespace
TEST_CASE("RISCV ASM code generator symbol-only lines",
          "[scope:core][scope:core.langs][level:asmb3][level:asmb5][kind:unit][arch:riscv]") {
  using Parser = pepp::tc::parser::RISCVParser;
  using Type = pepp::core::symbol::Type;
  using namespace pepp::tc;
  using Symbols = std::map<std::string, std::shared_ptr<pepp::core::symbol::Entry>>;
  // Assemble, assign addresses, and convert to object code, returning all symbol declarations.
  auto assemble = [](const char *source, u32 &total_size) {
    pepp::tc::DiagnosticTable diag;
    auto p = Parser(data(source));
    auto results = p.parse(diag);
    REQUIRE(diag.count() == 0);
    auto split = pepp::tc::riscv_split_to_sections(diag, results);
    REQUIRE(diag.count() == 0);
    auto &sections = split.grouped_ir;
    auto addresses = pepp::tc::riscv_assign_addresses(sections, 0x100);
    total_size = 0;
    for (const auto &pair : addresses.container) total_size += pair.second.size;
    (void)pepp::tc::riscv_to_object_code(diag, addresses, sections);
    Symbols ret;
    for (const auto &line : results)
      if (auto sym = line->typed_attribute<SymbolDeclaration>(); sym) ret[std::string(sym->entry->name)] = sym->entry;
    return ret;
  };

  const auto check = [](const Symbols &symbols, const std::vector<Expect> &expected) {
    REQUIRE(symbols.size() == expected.size());
    for (const auto &e : expected) {
      CAPTURE(e.name);
      REQUIRE(symbols.count(e.name) == 1);
      const auto &value = *symbols.at(e.name)->value;
      CHECK(value.value()() == e.address);
      CHECK(value.size() == e.size);
      CHECK(value.type() == e.type);
    }
  };

  SECTION("symbols inherit their target's location") {
    u32 size = 0;
    // Sections run A B A, with labels at the tail of the first A and code after them in the second A.
    auto symbols = assemble(".SECTION \".text\", \"rwx\"\n"
                            "add x1, x2, x3\n"      // 0x100
                            "a:\nb:\n# comment\n\n" // a, b: target the sub, across comments and blanks
                            "sub x1, x2, x3\n"      // 0x104
                            "c:\n"                  // alias of d
                            "d: add x1, x2, x3\n"   // 0x108
                            "f:\ng:\n"              // Points to the trailing sub in re-opened section
                            ".SECTION \".data\", \"rw\"\n"
                            "e:\n"      // target is data, not code
                            ".WORD 5\n" // 0x110
                            "h:\n"      // tail of B
                            ".SECTION \".text\", \"rwx\"\n"
                            "sub x1, x2, x3\n", // 0x10c, after the tail labels
                            size);
    check(symbols, {{"a", 0x104, 4, Type::Code},
                    {"b", 0x104, 4, Type::Code},
                    {"c", 0x108, 4, Type::Code},
                    {"d", 0x108, 4, Type::Code},
                    {"f", 0x10c, 4, Type::Code},
                    {"g", 0x10c, 4, Type::Code},
                    {"e", 0x110, 4, Type::Object},
                    {"h", 0x114, 0, Type::Object}});
    // Labels share a location but remain distinct symbols.
    CHECK(symbols.at("a") != symbols.at("b"));
    CHECK(dynamic_cast<pepp::core::symbol::AliasValue *>(symbols.at("c")->value.get()));
    CHECK(!dynamic_cast<pepp::core::symbol::AliasValue *>(symbols.at("d")->value.get()));
    CHECK(size == 20); // Symbol-only lines occupy no space.
  }
  SECTION("symbols before .ORG") {
    u32 size = 0;
    auto symbols = assemble("add x1, x2, x3\n.ORG 0x200\nsub x1, x2, x3\nfoo:\n.ORG 0x400\nadd x1, x2, x3", size);
    check(symbols, {{"foo", 0x204, 0, Type::Object}});
  }
}

TEST_CASE("RISCV ASM code generator relocations",
          "[scope:core][scope:core.langs][level:asmb3][level:asmb5][kind:unit][arch:riscv]") {
  using Parser = pepp::tc::parser::RISCVParser;
  using namespace pepp::tc;
  using enum pepp::bts::RelocationsRISCV;
  struct Assembled {
    DiagnosticTable diag;
    // Keep the symbol table alive after parsing to prevent dangling pointers in relocations
    std::shared_ptr<pepp::core::symbol::LeafTable> symbols;
    IRProgram program;
    RISCVSectionAnalysisResults split;
    IRMemoryAddressTable<RISCVAddress> addresses;
    ProgramObjectCodeResult code;
  };
  const auto assemble = [](const char *source) {
    Assembled ret;
    auto p = Parser(data(source));
    ret.symbols = p.symbol_table();
    ret.program = p.parse(ret.diag);
    REQUIRE(ret.diag.count() == 0);
    ret.split = riscv_split_to_sections(ret.diag, ret.program);
    ret.addresses = riscv_assign_addresses(ret.split.grouped_ir);
    ret.code = riscv_to_object_code(ret.diag, ret.addresses, ret.split.grouped_ir);
    return ret;
  };
  // One use of each fixup rule. data is 0x1800 past the auipc, so its lower 12 bits (0x800) sign-extend negative.
  auto result = assemble(R"(k: .equ 5
beq x1, x2, ext
jal x1, ext + 4
lui x1, %hi(data)
addi x1, x1, %lo(data + 8)
sw x1, %lo(data)(x2)
.Lhi: auipc x1, %pcrel_hi(data)
addi x1, x1, %pcrel_lo(.Lhi)
sw x1, %pcrel_lo(.Lhi)(x2)
.word ext
lui x1, %hi(k)
.Lunused: nop
.skip 0x17E8
data: .word 0)");
  REQUIRE(result.diag.count() == 0);
  SECTION("instruction types+operand patterns that we can relocate") {
    struct Expected {
      u32 offset;
      pepp::bts::RelocationsRISCV type;
      std::string symbol;
      i64 addend;
    };
    // A constant (k) generates no relocations
    const std::vector<Expected> expected{
        {0, R_RISCV_BRANCH, "ext", 0},       {4, R_RISCV_JAL, "ext", 4},
        {8, R_RISCV_HI20, "data", 0},        {12, R_RISCV_LO12_I, "data", 8},
        {16, R_RISCV_LO12_S, "data", 0},     {20, R_RISCV_PCREL_HI20, "data", 0},
        {24, R_RISCV_PCREL_LO12_I, ".Lhi", 0}, {28, R_RISCV_PCREL_LO12_S, ".Lhi", 0},
        {32, R_RISCV_32, "ext", 0},
    };
    const auto &relocations = result.code.relocations.at(0);
    REQUIRE(relocations.size() == expected.size());
    for (std::size_t it = 0; it < expected.size(); it++) {
      CAPTURE(it);
      CHECK(relocations[it].section_offset == expected[it].offset);
      CHECK(relocations[it].type == bits::to_underlying(expected[it].type));
      CHECK(relocations[it].target.symbol->name == expected[it].symbol);
      CHECK(relocations[it].target.addend == expected[it].addend);
    }
  }
  SECTION("%pcrel exhibits correct sign-extension behavior") {
    const auto word = [&](u32 offset) {
      const auto bytes = result.code.section_slices.at(0).get(offset, 4);
      return u32(bytes[0]) | u32(bytes[1]) << 8 | u32(bytes[2]) << 16 | u32(bytes[3]) << 24;
    };
    CHECK(word(20) >> 12 == 2);
    CHECK(word(24) >> 20 == 0x800);                                     // I-type imm[11:0]
    CHECK((word(28) >> 25 == 0x40 && ((word(28) >> 7) & 0x1F) == 0)); // S-type imm[11:5], imm[4:0]
  }
  SECTION("Local labels are removed unless they are a target of a relocation") {
    auto written = riscv_to_elf(result.split.grouped_ir, result.program, result.addresses, result.code, *result.symbols);
    const auto bytes = elf_bytes(written);
    std::istringstream in(std::string(reinterpret_cast<const char *>(bytes.data()), bytes.size()));
    ELFIO::elfio elf;
    REQUIRE(elf.load(in));
    const ELFIO::symbol_section_accessor symbols(elf, elf.sections[".symtab"]);
    const auto name_of = [&](ELFIO::Elf_Xword index) {
      std::string name;
      ELFIO::Elf64_Addr value;
      ELFIO::Elf_Xword size;
      unsigned char bind, type, other;
      ELFIO::Elf_Half shndx;
      symbols.get_symbol(index, name, value, size, bind, type, shndx, other);
      return name;
    };
    std::vector<std::string> names;
    for (ELFIO::Elf_Xword it = 0; it < symbols.get_symbols_num(); it++) names.push_back(name_of(it));
    CHECK(std::ranges::contains(names, ".Lhi"));
    CHECK(!std::ranges::contains(names, ".Lunused"));
    CHECK(std::ranges::contains(names, "data"));
    // The %pcrel_lo references .Lhi rather than its section+offset.
    const ELFIO::relocation_section_accessor relocations(elf, elf.sections[".rela.text"]);
    ELFIO::Elf64_Addr offset;
    ELFIO::Elf_Word symbol;
    unsigned type;
    ELFIO::Elf_Sxword addend;
    REQUIRE(relocations.get_entry(6, offset, symbol, type, addend));
    CHECK((offset == 24 && type == bits::to_underlying(R_RISCV_PCREL_LO12_I)));
    CHECK((name_of(symbol) == ".Lhi" && addend == 0));
  }
  SECTION("Invalid relocations") {
    using E = CodegenError;
    using enum CodegenError::NullaryError;
    // The accepted forms of each are covered above.
    for (const auto &[source, error] : std::vector<std::pair<const char *, E::NullaryError>>{
             {"addi x1, x1, %hi(l)\nl:", Argument_NotRelocatable},                  // Wrong instruction type for %hi
             {".word l * 2\nl:", Argument_NotRelocatable},                          // Not symbol [+ constant]
             {"addi x1, x1, %pcrel_lo(l)\nl: nop", Argument_UnpairedPcrelLo},       // Not an auipc
             {"l: lui x1, 1\naddi x1, x1, %pcrel_lo(l)", Argument_UnpairedPcrelLo}, // No %pcrel_hi
         }) {
      CAPTURE(source);
      const auto rejected = assemble(source);
      REQUIRE(rejected.diag.count() == 1);
      CHECK(rejected.diag.cbegin()->second == E::to_string(error));
    }
  }
}
