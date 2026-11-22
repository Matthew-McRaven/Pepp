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
#include "core/link/relocations.hpp"
#include <catch.hpp>
#include <optional>
#include <span>
#include <utility>
#include <variant>
#include <vector>
#include "./helpers.hpp"

namespace {
using namespace link_test;
using Failures = std::vector<std::pair<RelocationCode, u64>>;

u64 offset_of(const ResolvedRelocation &rel) { return rel.offset; }
u64 offset_of(const PairedRelocation &paired) { return paired.rel.offset; }

template <ElfEndian E>
Failures relocation_failures(ElfMachineType machine, u64 address, std::span<u8> bytes,
                             const std::vector<std::variant<ResolvedRelocation, PairedRelocation>> &relocations) {
  Failures ret;
  for (const auto &relocation : relocations)
    if (const auto code = apply_relocation<ElfBits::b32, E>(machine, address, bytes, relocation))
      ret.emplace_back(*code, std::visit([](const auto &it) { return offset_of(it); }, relocation));
  return ret;
}
} // namespace

TEST_CASE("apply_relocation unit tests", "[scope:core][scope:core.link][kind:unit][arch:*]") {
  using namespace link_test;
  using enum RelocationsPep;
  using enum ElfMachineType;
  using Result = std::pair<std::optional<RelocationCode>, std::vector<u8>>;
  const auto apply = [](ElfMachineType machine, RelocationsPep type, u64 symbol, i64 addend, u64 offset) {
    std::vector<u8> bytes(6, 0xEE);
    const ResolvedRelocation rel{offset, bits::to_underlying(type), symbol, addend};
    const auto failures = relocation_failures<ElfEndian::be>(machine, 0, bytes, {rel});
    return Result{failures.empty() ? std::nullopt : std::optional(failures[0].first), bytes};
  };
  const std::optional<RelocationCode> ok;
  CHECK(apply(EM_PEP10, R_PEP10_ABS16, 0x1230, 4, 1) == Result{ok, {0xEE, 0x12, 0x34, 0xEE, 0xEE, 0xEE}});
  CHECK(apply(EM_PEP9, R_PEP10_ABS32, 0x10, -1, 2) == Result{ok, {0xEE, 0xEE, 0, 0, 0, 0x0F}});
  // A field holds a value that fits as either signed or unsigned.
  CHECK(apply(EM_PEP10, R_PEP10_ABS8, 0xFF, 0, 0).first == ok);
  CHECK(apply(EM_PEP10, R_PEP10_ABS8, 0, -128, 0) == Result{ok, {0x80, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE}});
  CHECK(apply(EM_PEP10, R_PEP10_ABS8, 0x100, 0, 0).first == RelocationCode::Truncated);
  CHECK(apply(EM_PEP10, R_PEP10_ABS16, 0, -32769, 0).first == RelocationCode::Truncated);
  CHECK(apply(EM_PEP10, R_PEP10_ABS16, 0, 0, 5).first == RelocationCode::BadOffset);
  CHECK(apply(EM_PEP10, R_PEP10_NONE, 0, 0, 9).first == ok);
  CHECK(apply(EM_PEP10, static_cast<RelocationsPep>(99), 0, 0, 0).first == RelocationCode::UnsupportedType);
  // RISC-V data follows the file's endianness.
  const auto riscv = [&](RelocationsRISCV type) { return apply(EM_RISCV, static_cast<RelocationsPep>(type), 8, 0, 0); };
  CHECK(riscv(RelocationsRISCV::R_RISCV_32) == Result{ok, {0, 0, 0, 8, 0xEE, 0xEE}});
  CHECK(apply(EM_NONE, R_PEP10_ABS16, 0, 0, 0).first == RelocationCode::UnsupportedType);
}

TEST_CASE("apply_relocation RISC-V", "[scope:core][scope:core.link][kind:unit][arch:rv]") {
  using namespace link_test;
  using enum RelocationsRISCV;
  const auto rel = [](u64 offset, RelocationsRISCV type, std::optional<u64> symbol, i64 addend = 0) {
    return ResolvedRelocation{offset, bits::to_underlying(type), symbol, addend};
  };
  const auto relocate = [](std::span<u8> bytes,
                           const std::vector<std::variant<ResolvedRelocation, PairedRelocation>> &relocations) {
    return relocation_failures<ElfEndian::le>(ElfMachineType::EM_RISCV, 0x1000, bytes, relocations);
  };

  SECTION("Instruction and data fields") {
    // Instructions have zero'd immediates.
    std::vector<u8> bytes{
        0x63, 0x80, 0x20, 0x00, // beq x1, x2
        0xEF, 0x00, 0x00, 0x00, // jal x1
        0x97, 0x00, 0x00, 0x00, // auipc x1
        0x03, 0xA1, 0x00, 0x00, // lw x2, (x1)
        0x23, 0xA0, 0x20, 0x00, // sw x2, (x1)
        0xB7, 0x01, 0x00, 0x00, // lui x3
        0x93, 0x81, 0x01, 0x00, // addi x3, x3
        0x00, 0x00, 0x00, 0x00, // .word
        0x10, 0x00, 0x00, 0x00, // .word 0x10
        0x10, 0x00,             // .half 0x10
        0x05, 0xEE,             // .byte 5, unrelocated
        0x97, 0x00, 0x00, 0x00, // auipc x1
        0xE7, 0x80, 0x00, 0x00, // jalr x1, (x1)
    };
    const auto high = rel(8, R_RISCV_PCREL_HI20, 0x3000);
    const auto store = rel(16, R_RISCV_PCREL_LO12_S, 0x1000, 8);
    // S + A of an R_RISCV_PCREL_LO12_S is its auipc's address, which is 8 bytes into the section.
    const auto key = paired_key(ElfMachineType::EM_RISCV, 0x1000, store);
    REQUIRE(key.has_value());
    CHECK(key->offset == high.offset);
    CHECK(key->type == high.type);
    CHECK_FALSE(paired_key(ElfMachineType::EM_RISCV, 0x1000, high).has_value());
    // The auipcs and lui round up, because their paired lower 12 bits sign-extend negative.
    const std::vector<std::variant<ResolvedRelocation, PairedRelocation>> relocations{
        rel(0, R_RISCV_BRANCH, 0xF00),
        rel(4, R_RISCV_JAL, 0x2000, 4),
        high,
        PairedRelocation{rel(12, R_RISCV_PCREL_LO12_I, 0x1008), high},
        PairedRelocation{store, high},
        rel(20, R_RISCV_HI20, 0x1234'5800),
        rel(24, R_RISCV_LO12_I, 0x1234'5800),
        rel(28, R_RISCV_32_PCREL, 0x900),
        rel(32, R_RISCV_ADD32, 0x2000),
        rel(32, R_RISCV_SUB32, 0x1800),
        rel(36, R_RISCV_ADD16, 0x30),
        rel(36, R_RISCV_SUB16, 0x20),
        rel(38, R_RISCV_ADD8, 0x10),
        rel(38, R_RISCV_SUB8, 3),
        rel(40, R_RISCV_CALL_PLT, 0x2830),
    };
    CHECK(relocate(bytes, relocations).empty());
    // Checked against riscv64-elf-objdump with --adjust-vma=0x1000.
    const std::vector<u8> expected{
        0xE3, 0x80, 0x20, 0xF0, // beq ra, sp, 0xf00
        0xEF, 0x10, 0x00, 0x00, // jal ra, 0x2004
        0x97, 0x20, 0x00, 0x00, // auipc ra, 0x2
        0x03, 0xA1, 0x80, 0xFF, // lw sp, -8(ra) # 0x3000
        0x23, 0xAC, 0x20, 0xFE, // sw sp, -8(ra)
        0xB7, 0x61, 0x34, 0x12, // lui gp, 0x12346
        0x93, 0x81, 0x01, 0x80, // addi gp, gp, -2048 # 0x12345800
        0xE4, 0xF8, 0xFF, 0xFF, // 0x900 - 0x101c
        0x10, 0x08, 0x00, 0x00, // 0x10 + 0x2000 - 0x1800
        0x20, 0x00,             // 0x10 + 0x30 - 0x20
        0x12, 0xEE,             // 5 + 0x10 - 3
        0x97, 0x20, 0x00, 0x00, // auipc ra, 0x2
        0xE7, 0x80, 0x80, 0x80, // jalr -2040(ra) # 0x2830
    };
    CHECK(bytes == expected);
  }

  SECTION("Failures") {
    std::vector<u8> bytes(28, 0);
    const auto high = rel(12, R_RISCV_PCREL_HI20, std::nullopt);
    const std::vector<std::variant<ResolvedRelocation, PairedRelocation>> relocations{
        rel(0, R_RISCV_BRANCH, 0x2000),
        rel(4, R_RISCV_JAL, 0x1005),
        // An R_RISCV_PCREL_LO12_I without its R_RISCV_PCREL_HI20.
        rel(8, R_RISCV_PCREL_LO12_I, 0x1000),
        // The R_RISCV_PCREL_HI20 has no address, so neither does its R_RISCV_PCREL_LO12_I.
        high,
        PairedRelocation{rel(16, R_RISCV_PCREL_LO12_I, 0x100C), high},
        rel(20, R_RISCV_ALIGN, 0),
        rel(26, R_RISCV_LO12_I, 0),
    };
    using enum RelocationCode;
    CHECK(relocate(bytes, relocations) == Failures{{Truncated, 0},
                                                   {Misaligned, 4},
                                                   {Unpaired, 8},
                                                   {NoSymbolAddress, 12},
                                                   {NoSymbolAddress, 16},
                                                   {UnsupportedType, 20},
                                                   {BadOffset, 26}});
  }
}
