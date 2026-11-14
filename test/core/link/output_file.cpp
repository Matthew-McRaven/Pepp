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
#include <memory>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>
#include "./helpers.hpp"
#include "core/formats/elf/packed_io.hpp"
#include "core/langs/asmb/diagnostic_table.hpp"
#include "core/langs/ldscript/parser.hpp"
#include "core/link/linker_passes.hpp"

namespace {
using namespace link_test;

// Returns an object with null, .shstrtab, then a section for each spec, so that the first spec is section 2.
AnyInputElf object(std::initializer_list<SectionSpec> specs) {
  auto elf = elf_with(specs);
  return to_input_elf(elf);
}

// Link two objects with a script, which has both a gap between members and padding after the last one.
LinkContext link() {
  using enum SectionFlags;
  using enum SectionTypes;
  using namespace bits;
  pepp::tc::DiagnosticTable diag;
  auto script = pepp::tc::ldscript::parse(diag, R"(SECTIONS {
  .text 0x100 : { *(.text) . = ALIGN(8); }
  .data : { *(.data) }
  .none : { *(.none) }
  .bss : { *(.bss) }
})");
  REQUIRE(diag.count() == 0);
  LinkConfig config;
  config.inputs.push_back({LinkInput::Script{std::move(script)}});
  config.inputs.push_back({object({{".text", SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR, 0, 1, {1, 2, 3}},
                                   {".bss", SHT_NOBITS, SHF_ALLOC | SHF_WRITE, 4}})});
  config.inputs.push_back({object({{".text", SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR, 0, 4, {4, 5}},
                                   {".data", SHT_PROGBITS, SHF_ALLOC | SHF_WRITE, 0, 1, {6}}})});
  return do_link(std::move(config));
}

// Returns the output of a link as an input file.
template <ElfBits B, ElfEndian E> PackedInputElfFile<B, E> reread(const LinkContext &context) {
  REQUIRE(context.output);
  return PackedInputElfFile<B, E>(context.output);
}
} // namespace

TEST_CASE("Executable linker outputs", "[scope:core][scope:core.link][kind:int][arch:*]") {
  using enum SectionFlags;
  using enum SectionTypes;
  using enum SegmentFlags;
  using namespace bits;
  using Section = std::tuple<std::string_view, u32, u32, u32, u32>;
  using Segment = std::tuple<u32, u32, u32, u32>;
  auto result = link();
  CHECK(result.diagnostics.size() == 0);
  const auto elf = reread<ElfBits::b32, ElfEndian::le>(result);

  CHECK(elf.header.e_type == to_underlying(ElfFileType::ET_EXEC));
  CHECK(elf.header.e_machine == to_underlying(ElfMachineType::EM_PEP10));

  // The empty .none is discarded.
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
  // The second member is aligned to 4, and ALIGN(8) pads the end of the section.
  const auto text = section_bytes(elf, 2);
  CHECK(std::vector<u8>(text.begin(), text.end()) == std::vector<u8>{1, 2, 3, 0, 4, 5, 0, 0});

  // .text is loaded on its own. .bss extends the memory of .data's segment.
  std::vector<Segment> segments;
  for (const auto &header : elf.program_headers)
    segments.emplace_back(header.p_flags, header.p_vaddr, header.p_filesz, header.p_memsz);
  CHECK(segments == std::vector<Segment>{
                        {to_underlying(PF_R | PF_X), 0x100, 8, 8},
                        {to_underlying(PF_R | PF_W), 0x108, 1, 5},
                    });
}
