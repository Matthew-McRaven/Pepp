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
#include "core/link/map_file.hpp"
#include <catch.hpp>
#include <fstream>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <fmt/ranges.h>
#include "./helpers.hpp"
#include "core/langs/asmb/debug_line.hpp"
#include "core/langs/asmb/diagnostic_table.hpp"
#include "core/driver/asmb_driver.hpp"
#include "core/langs/ldscript/parser.hpp"
#include "core/link/linker_passes.hpp"
#include "spdlog/spdlog.h"

namespace {
using namespace link_test;
} // namespace

// Link a sample program with a dummy OS, showing how the user program's undefined symbols are eventually resolved by a
// linker.
TEST_CASE("Link a program with its OS", "[scope:core][scope:core.link][kind:e2e][arch:pep10]") {
  using namespace pepp::tc;
  const Pep10DriverConfig relocatable{.output_type = ElfFileType::ET_REL};
  auto program = assemble_pep10(relocatable, {.listing = true}, R"(.SECTION ".text","rx"
LDBA charIn,d
STBA charOut,d
RET
)");
  auto os = assemble_pep10(relocatable, {}, R"(.SECTION ".os","rw"
.EXPORT charIn
.EXPORT charOut
charIn:.BLOCK 1
charOut:.BLOCK 1
.SECTION ".a_long_section_name","rw"
.BLOCK 1
)");
  REQUIRE(program.ok());
  REQUIRE(os.ok());
  const auto listing = program.listing();
  REQUIRE(listing);
  DiagnosticTable diag;
  // Discard debug information to keep the link map short.
  auto script = ldscript::parse(diag, R"(SECTIONS {
  .text 0 : { *(.text) }
  .os 0xBEEF : { *(.os) }
  .none : { *(.none) }
  /DISCARD/ : { *(.debug*) }
})");
  REQUIRE(diag.count() == 0);
  const auto unlinked = reread<ElfBits::b32, ElfEndian::be>(program.elf()->elf, program.elf()->segments);

  LinkConfig config;
  config.inputs.push_back({LinkInput::Script{std::move(script)}});
  config.inputs.push_back({LinkInput::Opened{"program.o", to_input_elf(*program.elf())}});
  config.inputs.push_back({LinkInput::Opened{"os.o", to_input_elf(*os.elf())}});
  config.map = "link_map_os.map";
  auto result = do_link(std::move(config));
  CHECK(result.diagnostics.size() == 0);
  const auto linked = reread<ElfBits::b32, ElfEndian::be>(result);

  // Bytes 1-2 and 4-5 are the operands of LDBA and STBA. The opcodes and RET do not change.
  const auto text_of = [](const auto &elf) {
    const auto text = section_bytes(elf, find_named_section(elf, ".text"));
    return std::vector<u8>(text.begin(), text.end());
  };
  const auto before = text_of(unlinked), after = text_of(linked);
  REQUIRE(before.size() == 7);
  REQUIRE(after.size() == 7);
  CHECK(std::vector<u8>{before[1], before[2], before[4], before[5]} == std::vector<u8>{0x00, 0x00, 0x00, 0x00});
  CHECK(std::vector<u8>{after[1], after[2], after[4], after[5]} == std::vector<u8>{0xBE, 0xEF, 0xBE, 0xF0});
  CHECK(std::vector<u8>{after[0], after[3], after[6]} == std::vector<u8>{before[0], before[3], before[6]});

  CHECK(!link_map(result)->empty());
  std::ifstream written("link_map_os.map", std::ios::binary);
  CHECK(std::string(std::istreambuf_iterator<char>(written), {}) == link_map(result));
  // program.o is the first ELF file in the link. Its discarded debug information has no address.
  CHECK(section_address(result, {pepp::bts::ElfFileID(1), find_named_section(unlinked, ".debug_line")}) ==
        std::nullopt);

  // Move each listing row from its offset in program.o to its address in the output, which holds the relocated bytes.
  auto rows = read_listing_rows(unlinked);
  REQUIRE(rows);
  for (auto &row : *rows) {
    const auto address = section_address(result, {pepp::bts::ElfFileID(1), row.section});
    REQUIRE(address);
    row.section = bits::to_underlying(SectionIndices::SHN_ABS);
    row.start += *address, row.end += *address;
  }
  REQUIRE(program.unformatted_listing);
  const auto &[text, style] = *program.unformatted_listing;
  const auto relinked = format_listing(std::make_unique<PackedInputElfBE32>(result.output), *rows, text, style);
  REQUIRE(relinked);
  SPDLOG_WARN("Pre-link:\n{}", fmt::join(*listing, "\n"));
  SPDLOG_WARN("Post-link:\n{}", fmt::join(*relinked, "\n"));
  SPDLOG_WARN("Link map:\n{}", *link_map(result));
}
