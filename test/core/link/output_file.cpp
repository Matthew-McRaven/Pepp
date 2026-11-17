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
#include "core/link/output_file.hpp"
#include <catch.hpp>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>
#include "./helpers.hpp"
#include "core/formats/elf/packed_io.hpp"
#include "core/langs/asmb/diagnostic_table.hpp"
#include "core/langs/asmb_driver.hpp"
#include "core/langs/ldscript/parser.hpp"
#include "core/link/linker_passes.hpp"

namespace {
using namespace link_test;

// Link two objects with a script, which has both a gap between members and padding after the last one. The first
// object's .text is relocated against symbol. The second object defines value in .data, none in an empty section, and
// dead in a discarded one.
LinkContext link(std::optional<std::string> entry, RelocationsPep type = RelocationsPep::R_PEP10_ABS16,
                 std::string_view symbol = "value") {
  using enum SectionFlags;
  using enum SectionTypes;
  using namespace bits;
  pepp::tc::DiagnosticTable diag;
  auto script = pepp::tc::ldscript::parse(diag, R"(ENTRY(main)
SECTIONS {
  .text 0x100 : { *(.text) . = ALIGN(8); }
  .data : { *(.data) }
  .none : { *(.none) }
  .bss : { *(.bss) }
  /DISCARD/ : { *(.gone) }
})");
  REQUIRE(diag.count() == 0);
  LinkConfig config;
  config.entry = std::move(entry);
  config.inputs.push_back({LinkInput::Script{std::move(script)}});
  config.inputs.push_back({object_with({{".text", SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR, 0, 1, {1, 2, 3}},
                                        {".bss", SHT_NOBITS, SHF_ALLOC | SHF_WRITE, 4}},
                                       {{"main", 2, 1}, {"value"}, {"none"}, {"dead"}}, {{2, 1, type, symbol, 2}})});
  config.inputs.push_back({object_with({{".text", SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR, 0, 4, {4, 5}},
                                        {".data", SHT_PROGBITS, SHF_ALLOC | SHF_WRITE, 0, 1, {6}},
                                        {".none", SHT_PROGBITS, SHF_ALLOC | SHF_WRITE},
                                        {".gone", SHT_PROGBITS, SHF_ALLOC | SHF_WRITE, 0, 1, {9}}},
                                       {{"value", 3, 0}, {"none", 4, 0}, {"dead", 5, 0}})});
  return do_link(std::move(config));
}

// Returns the entry point of a link's output.
u32 entry_point(const LinkContext &context) { return reread<ElfBits::b32, ElfEndian::le>(context).header.e_entry; }

// Returns the value of each symbol that has a name and is not a section symbol.
template <ElfBits B, ElfEndian E> std::map<std::string, u64> symbol_values(const PackedInputElfFile<B, E> &elf) {
  std::map<std::string, u64> ret;
  const auto index = find_named_section(elf, ".symtab");
  REQUIRE(index != 0);
  const PackedSymbolReader<B, E> symtab(elf, index);
  for (u32 it = 1; it < symtab.symbol_count(); ++it) {
    const auto name = symtab.get_symbol_name(it);
    if (const auto symbol = symtab.get_symbol(it); symbol.st_type() != SymbolType::STT_SECTION && !name.empty())
      ret[std::string(name.data(), name.size())] = symbol.st_value;
  }
  return ret;
}
} // namespace

TEST_CASE("Executable linker outputs", "[scope:core][scope:core.link][kind:int][arch:*]") {
  using enum SectionFlags;
  using enum SectionTypes;
  using enum SegmentFlags;
  using enum SymbolType;
  using namespace bits;
  using Section = std::tuple<std::string_view, u32, u32, u32, u32>;
  using Segment = std::tuple<u32, u32, u32, u32>;
  using Symbol = std::tuple<std::string_view, SymbolType, u16, u32>;
  auto result = link(std::nullopt);
  CHECK(result.diagnostics.size() == 0);
  const auto elf = reread<ElfBits::b32, ElfEndian::le>(result);

  CHECK(elf.header.e_type == to_underlying(ElfFileType::ET_EXEC));
  CHECK(elf.header.e_machine == to_underlying(ElfMachineType::EM_PEP10));
  // main is 1 byte into the first object's .text.
  CHECK(elf.header.e_entry == 0x101);

  // The empty .none is discarded. The output sections come before .strtab and .symtab.
  std::vector<Section> sections;
  for (u16 index = 2; index < 5; ++index) {
    const auto &header = elf.section_headers[index];
    sections.emplace_back(section_name(elf, index), header.sh_type, header.sh_flags, header.sh_addr, header.sh_size);
  }
  CHECK(sections == std::vector<Section>{
                        {".text", to_underlying(SHT_PROGBITS), to_underlying(SHF_ALLOC | SHF_EXECINSTR), 0x100, 8},
                        {".data", to_underlying(SHT_PROGBITS), to_underlying(SHF_ALLOC | SHF_WRITE), 0x108, 1},
                        {".bss", to_underlying(SHT_NOBITS), to_underlying(SHF_ALLOC | SHF_WRITE), 0x109, 4},
                    });
  CHECK(elf.section_headers[2].sh_addralign == 4);
  // The second member is aligned to 4, and ALIGN(8) pads the end of the section. The relocation wrote value + 2,
  // which is 0x10a, over bytes 1 and 2 in the file's byte order.
  const auto text = section_bytes(elf, 2);
  CHECK(std::vector<u8>(text.begin(), text.end()) == std::vector<u8>{1, 0x0a, 0x01, 0, 4, 5, 0, 0});

  // .text is loaded on its own. .bss extends the memory of .data's segment.
  std::vector<Segment> segments;
  for (const auto &header : elf.program_headers)
    segments.emplace_back(header.p_flags, header.p_vaddr, header.p_filesz, header.p_memsz);
  CHECK(segments == std::vector<Segment>{
                        {to_underlying(PF_R | PF_X), 0x100, 8, 8},
                        {to_underlying(PF_R | PF_W), 0x108, 1, 5},
                    });

  // A section symbol for each output section, then each global.
  const auto index = find_named_section(elf, ".symtab");
  REQUIRE(index != 0);
  CHECK(elf.section_headers[index].sh_info == 4);
  const PackedSymbolReader<ElfBits::b32, ElfEndian::le> symtab(elf, index);
  std::vector<Symbol> symbols;
  for (u32 it = 1; it < symtab.symbol_count(); ++it) {
    const auto symbol = symtab.get_symbol(it);
    const auto name = symtab.get_symbol_name(it);
    symbols.emplace_back(std::string_view(name.data(), name.size()), symbol.st_type(), symbol.st_shndx,
                         symbol.st_value);
  }
  CHECK(symbols == std::vector<Symbol>{
                       {"", STT_SECTION, 2, 0x100},
                       {"", STT_SECTION, 3, 0x108},
                       {"", STT_SECTION, 4, 0x109},
                       {"main", STT_NOTYPE, 2, 0x101},
                       {"value", STT_NOTYPE, 3, 0x108},
                   });

  // A missing entry symbol from -e is a warning, and the entry point becomes the address of .text.
  auto fallback = link("missing");
  CHECK(entries_of(fallback.diagnostics) ==
        std::vector<Diagnostic>{{Severity::Warning, LinkCode::NoEntrySymbol, EntryFallback{"missing", 0x100}}});
  CHECK(entry_point(fallback) == 0x100);
  // An entry that is not a symbol is read as an integer literal.
  CHECK(entry_point(link("10")) == 10);

  // Truncated values are errors.
  auto truncated = link(std::nullopt, RelocationsPep::R_PEP10_ABS8);
  CHECK(entries_of(truncated.diagnostics) ==
        std::vector<Diagnostic>{{Severity::Error, RelocationCode::Truncated, RelocationSite{{ElfFileID{1}, 2}, 1}}});

  // Though the symbol is in an empty output section it still has an address of of 0x109. Relocation adds plus 2.
  auto empty = link(std::nullopt, RelocationsPep::R_PEP10_ABS16, "none");
  const auto relocated = reread<ElfBits::b32, ElfEndian::le>(empty);
  CHECK(section_bytes(relocated, 2)[1] == 0x0b);
  // A symbol in a discarded section has no address.
  auto discarded = link(std::nullopt, RelocationsPep::R_PEP10_ABS16, "dead");
  CHECK(entries_of(discarded.diagnostics) == std::vector<Diagnostic>{{Severity::Error, RelocationCode::NoSymbolAddress,
                                                                      RelocationSite{{ElfFileID{1}, 2}, 1}}});
}

// Link an object whose .text is relocated against call, which an executable defines 3 bytes into its .text at 0x8000.
TEST_CASE("Executable inputs", "[scope:core][scope:core.link][kind:int][arch:pep10]") {
  using enum SectionFlags;
  using enum SectionTypes;
  using namespace bits;
  const auto link = [](bool just_symbols, std::initializer_list<SymbolSpec> symbols) {
    pepp::tc::DiagnosticTable diag;
    auto script = pepp::tc::ldscript::parse(diag, "SECTIONS { .text 0x100 : { *(.text) } }");
    REQUIRE(diag.count() == 0);
    LinkConfig config;
    config.inputs.push_back({LinkInput::Script{std::move(script)}});
    config.inputs.push_back({object_with({{".text", SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR, 0, 1, {1, 2, 3}}},
                                         {{"call"}}, {{2, 1, RelocationsPep::R_PEP10_ABS16, "call"}})});
    config.inputs.push_back(
        {object_with({{".text", SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR, 0, 1, {7, 7, 7, 7}, 0x8000}}, symbols, {},
                     ElfFileType::ET_EXEC),
         just_symbols});
    const auto result = do_link(std::move(config));
    CHECK(result.diagnostics.size() == 0);
    return reread<ElfBits::b32, ElfEndian::le>(result);
  };
  const auto text_of = [](const auto &elf) {
    const auto text = section_bytes(elf, 2);
    return std::vector<u8>(text.begin(), text.end());
  };

  // The executable's .text follows the object's at 0x103, so call is at 0x106.
  const auto placed = link(false, {{"call", 2, 0x8003}});
  CHECK(symbol_values(placed).at("call") == 0x106);
  CHECK(text_of(placed) == std::vector<u8>{1, 0x06, 0x01, 7, 7, 7, 7});

  // -R does not insert sections but keeps symbols as absolutes. The undefined missing is not an error.
  const auto just_symbols = link(true, {{"call", 2, 0x8003}, {"missing"}});
  CHECK(symbol_values(just_symbols).at("call") == 0x8003);
  CHECK(text_of(just_symbols) == std::vector<u8>{1, 0x03, 0x80});
  const PackedSymbolReader<ElfBits::b32, ElfEndian::le> symtab(just_symbols,
                                                                find_named_section(just_symbols, ".symtab"));
  CHECK(u16(symtab.get_symbol(symtab.find_symbol("call")).st_shndx) == to_underlying(SectionIndices::SHN_ABS));
}

// test that the assembler in "executable" mode produces the same result as "reloctable" mode with a link step.
// Linking the object must give the executable's addresses, contents, and symbol values, debug sections included.
TEST_CASE("linked object matches assembled executable", "[scope:core][scope:core.link][kind:e2e][arch:pep10]") {
  using namespace pepp::tc;
  const std::string source = R"(.SECTION ".text","rx"
main:LDWA num,d
ADDA 2,i
loop:STWA num,d
LDBA msg,d
BR loop
.SECTION ".data","rw"
num:.WORD 7
msg:.ASCII "Hi\x00"
ptr:.WORD loop
)";
  auto assembled = assemble_pep10(Pep10DriverConfig{}, {}, source);
  auto object = assemble_pep10(Pep10DriverConfig{.output_type = ElfFileType::ET_REL}, {}, source);
  REQUIRE(assembled.ok());
  REQUIRE(object.ok());
  const auto expected = reread<ElfBits::b32, ElfEndian::be>(assembled.elf()->elf, assembled.elf()->segments);

  LinkConfig config;
  config.inputs.push_back({to_input_elf(object.elf()->elf, &object.elf()->segments)});
  auto result = do_link(std::move(config));
  CHECK(result.diagnostics.size() == 0);
  const auto linked = reread<ElfBits::b32, ElfEndian::be>(result);

  u32 compared = 0;
  for (u16 index = 2; index < linked.section_headers.size(); ++index) {
    const auto name = section_name(linked, index);
    if (name == ".strtab" || name == ".symtab") continue;
    CAPTURE(name);
    const auto other = find_named_section(expected, name);
    REQUIRE(other != 0);
    CHECK(linked.section_headers[index].sh_addr == expected.section_headers[other].sh_addr);
    const auto ours = section_bytes(linked, index), theirs = section_bytes(expected, other);
    CHECK(std::vector<u8>(ours.begin(), ours.end()) == std::vector<u8>(theirs.begin(), theirs.end()));
    ++compared;
  }
  // .text, .data, and the debug sections which are not empty.
  CHECK(compared >= 6);
  CHECK(symbol_values(linked) == symbol_values(expected));
  CHECK(linked.header.e_entry == expected.header.e_entry);
}
