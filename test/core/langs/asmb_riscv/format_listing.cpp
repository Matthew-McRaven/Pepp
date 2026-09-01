/*
 * Copyright (c) 2026. Stanley Warford, Matthew McRaven
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <array>
#include <catch.hpp>
#include <fmt/format.h>
#include <string>
#include "core/arch/riscv/asmb/rv_mnemonics.hpp"
#include "core/arch/riscv/asmb/rvi_patterns.hpp"
#include "core/langs/asmb/diagnostic_table.hpp"
#include "core/langs/asmb_riscv/codegen.hpp"
#include "core/langs/asmb_riscv/parser.hpp"
#include "core/langs/asmb_riscv/text_format.hpp"

namespace {
static auto data = [](auto str) { return pepp::tc::support::SeekableData{std::string(str)}; };

// Assemble a program at a known base address and then format as a listing.
// Failure to assemble terminates test.
std::vector<std::string> listing_of(const std::string &source, u32 base) {
  pepp::tc::DiagnosticTable diag;
  auto parser = pepp::tc::parser::RISCVParser(data(source));
  auto lines = parser.parse(diag);
  CAPTURE(source);
  REQUIRE(diag.count() == 0);
  auto split = pepp::tc::riscv_split_to_sections(diag, lines);
  REQUIRE(diag.count() == 0);
  auto &sections = split.grouped_ir;
  REQUIRE(sections.size() == 1);
  const auto addresses = pepp::tc::riscv_assign_addresses(sections, base);
  const auto object_code = pepp::tc::riscv_to_object_code(addresses, sections);
  return pepp::tc::riscv_format_listing(sections[0].second, addresses, object_code);
}

// {:08X} would print out MSB first rather than LSB, so prefer individual byte access.
std::string object_code_column(ul32 word) {
  auto b = word.bytes_view();
  return fmt::format("{:02X}{:02X}{:02X}{:02X}", b[0], b[1], b[2], b[3]);
}

std::string row(u32 address, u32 word, const std::string &source) {
  return fmt::format("{:08X} {} {}", address, object_code_column(word), source);
}
} // namespace

TEST_CASE("RISCV ASM listing", "[scope:core][scope:core.langs][level:asmb3][level:asmb5][kind:unit][arch:riscv]") {
  SECTION("address, object code bytes, and source code") {
    const auto word = riscv::ADD.encode(riscv::Values{.rs1 = 2, .rs2 = 3, .rd = 1}).bits();
    const auto rows = listing_of("add x1, x2, x3", 0x1000);
    REQUIRE(rows.size() == 1);
    // Compare hand-built string against our row generator. We will use the row-style for more complex tests.
    CHECK(rows[0] == row(0x1000, word, "         add     x1, x2, x3"));
    CHECK(rows[0] == "00001000 B3003100          add     x1, x2, x3");
  }

  SECTION("addresses advance by one instruction per row") {
    const auto rows = listing_of("add x1, x2, x3\nsub x4, x5, x6\nlw x7, 8(x8)\n", 0x2000);
    REQUIRE(rows.size() == 3);
    const std::array<u32, 3> words{{
        riscv::ADD.encode(riscv::Values{.rs1 = 2, .rs2 = 3, .rd = 1}).bits(),
        riscv::SUB.encode(riscv::Values{.rs1 = 5, .rs2 = 6, .rd = 4}).bits(),
        riscv::LW.encode(riscv::Values{.rs1 = 8, .rd = 7, .imm = 8}).bits(),
    }};
    for (std::size_t i = 0; i < rows.size(); ++i) {
      CAPTURE(i, rows[i]);
      // Address column contains starts from correct offset.
      CHECK(rows[i].starts_with(fmt::format("{:08X} ", 0x2000 + 4 * i)));
      // Object code column agrees with the instruction encoding.
      CHECK(rows[i].substr(9, 8) == object_code_column(words[i]));
    }
  }

  SECTION("listing with continuation rows due to long object code.") {
    const auto rows = listing_of(".ascii \"abcdefghi\"", 0x1000);
    REQUIRE(rows.size() == 3);
    CHECK(rows[0] == "00001000 61626364          .ascii  \"abcdefghi\"");
    CHECK(rows[1] == "         65666768");
    CHECK(rows[2] == "         69");
  }
}
