/*
 * /Copyright (c) 2026. Stanley Warford, Matthew McRaven
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
#include <catch.hpp>
#include <cstdint>
#include <string>
#include "core/arch/riscv/asmb/rv_mnemonics.hpp"
#include "core/arch/riscv/isa/rv_instruction.hpp"
#include "core/arch/riscv/isa/rvi.hpp"

// These tests exist to ensure I don't re-introduce a bug while encoding the immediate.
// Check exhaustively since offsets are small.
TEST_CASE("RISC-V branch and jump immediates survive encode then decode",
          "[scope:core][scope:core.arch][kind:unit][arch:riscv]") {
  // riscv::string_to_mnemonic is a multiset. Use length as a discriminator to pick the right variant.
  // Check that the match is unique to avoid strange test failures.
  auto descriptor = [](const char *m, std::size_t operands) -> const riscv::MnemonicDescriptor & {
    const auto [first, last] = riscv::string_to_mnemonic.equal_range(std::string(m));
    const riscv::MnemonicDescriptor *found = nullptr;
    for (auto it = first; it != last; ++it) {
      if (it->mn.operands().size() != operands) continue;
      CAPTURE(m, operands);
      REQUIRE(found == nullptr);
      found = &it->mn;
    }
    CAPTURE(m, operands);
    REQUIRE(found != nullptr);
    return *found;
  };

  SECTION("B-type in range [-4096, 4094]") {
    for (auto [mnemonic, op] : {std::pair{"beq", RvOp::BEQ}, {"bne", RvOp::BNE}, {"blt", RvOp::BLT},
                                {"bge", RvOp::BGE}, {"bltu", RvOp::BLTU}, {"bgeu", RvOp::BGEU}}) {
      const auto &d = descriptor(mnemonic, 3);
      int32_t mismatches = 0, first_bad = 0;
      for (int32_t off = -4096; off <= 4094; off += 2) {
        riscv::Values v{.rs1 = uint8_t(11), .rs2 = uint8_t(12), .rd = std::nullopt, .imm = uint32_t(off)};
        const auto w = d.encode(v);
        const auto b = w.as<riscv::InstructionB>();
        // The other fields must survive too: a rotated immediate can bleed into them.
        if (b.signed_imm() != off || b.rs1 != 11 || b.rs2 != 12 || riscv::decode(w) != op) {
          if (mismatches++ == 0) first_bad = off;
        }
      }
      CAPTURE(mnemonic, mismatches, first_bad);
      CHECK(mismatches == 0);
    }
  }

  SECTION("J-type in range [-1048576, 1048574]") {
    const auto &d = descriptor("jal", 2);
    int32_t mismatches = 0, first_bad = 0;
    for (int32_t off = -1048576; off <= 1048574; off += 2) {
      riscv::Values v{.rs1 = std::nullopt, .rs2 = std::nullopt, .rd = uint8_t(10), .imm = uint32_t(off)};
      const auto w = d.encode(v);
      const auto j = w.as<riscv::InstructionJ>();
      if (j.jump_offset() != off || j.rd != 10 || riscv::decode(w) != RvOp::JAL) {
        if (mismatches++ == 0) first_bad = off;
      }
    }
    CAPTURE(mismatches, first_bad);
    CHECK(mismatches == 0);
  }
}

TEST_CASE("RISC-V imm_fits accepts exactly the representable immediates",
          "[scope:core][scope:core.arch][kind:unit][arch:riscv]") {
  // riscv::string_to_mnemonic is a multiset. Use length as a discriminator to pick the right variant.
  // Check that the match is unique to avoid strange test failures.
  auto descriptor = [](const char *m, std::size_t operands) -> const riscv::MnemonicDescriptor & {
    const auto [first, last] = riscv::string_to_mnemonic.equal_range(std::string(m));
    const riscv::MnemonicDescriptor *found = nullptr;
    for (auto it = first; it != last; ++it) {
      if (it->mn.operands().size() != operands) continue;
      CAPTURE(m, operands);
      REQUIRE(found == nullptr);
      found = &it->mn;
    }
    CAPTURE(m, operands);
    REQUIRE(found != nullptr);
    return *found;
  };

  SECTION("signed immediates span their whole range and stop there") {
    // {mnemonic, operand count, low, high, step}. A step of 2 marks the displacements whose low bit
    // is implicitly 0. operands needed to select correct variant of the mnemonic.
    for (auto [mnemonic, operands, lo, hi, step] : {std::tuple{"addi", 3, -2048, 2047, 1},
                                                    {"sw", 3, -2048, 2047, 1},
                                                    {"beq", 3, -4096, 4094, 2},
                                                    {"jal", 2, -1048576, 1048574, 2}}) {
      const auto &d = descriptor(mnemonic, operands);
      CAPTURE(mnemonic, lo, hi, step);
      CHECK(d.imm_fits(lo));
      CHECK(d.imm_fits(hi));
      CHECK(d.imm_fits(0));
      // One step past either end does not wrap or saturate.
      CHECK_FALSE(d.imm_fits(lo - step));
      CHECK_FALSE(d.imm_fits(hi + step));
      // An odd displacement is unrepresentable.
      if (step == 2) {
        CHECK_FALSE(d.imm_fits(1));
        CHECK_FALSE(d.imm_fits(-1));
        CHECK_FALSE(d.imm_fits(lo + 1));
      }
    }
  }

  SECTION("a U-type operand is an unsigned field, so its high half is legal") {
    const auto &d = descriptor("lui", 2);
    CHECK(d.imm_fits(0));
    CHECK(d.imm_fits(0x65));
    // Would read as negative under the signed rule the other types use.
    CHECK(d.imm_fits(0x80000));
    CHECK(d.imm_fits(0xFFFFF));
    CHECK_FALSE(d.imm_fits(0x100000));
    CHECK_FALSE(d.imm_fits(-1));
  }
}

