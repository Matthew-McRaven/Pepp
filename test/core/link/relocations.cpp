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
#include <utility>
#include <vector>
#include "./helpers.hpp"

TEST_CASE("apply_relocation unit tests", "[scope:core][scope:core.link][kind:unit][arch:*]") {
  using namespace link_test;
  using enum RelocationsPep;
  using enum ElfMachineType;
  using Result = std::pair<std::optional<RelocationCode>, std::vector<u8>>;
  const auto apply = [](ElfMachineType machine, RelocationsPep type, u64 symbol, i64 addend, u64 offset) {
    std::vector<u8> bytes(6, 0xEE);
    const ResolvedRelocation rel{offset, bits::to_underlying(type), symbol, addend};
    const auto error = apply_relocation(machine, ElfEndian::be, rel, bytes);
    return Result{error, bytes};
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
  // Only the RISC-V relocations for data words are supported.
  const auto riscv = [&](RelocationsRISCV type) { return apply(EM_RISCV, static_cast<RelocationsPep>(type), 8, 0, 0); };
  CHECK(riscv(RelocationsRISCV::R_RISCV_32) == Result{ok, {0, 0, 0, 8, 0xEE, 0xEE}});
  CHECK(riscv(RelocationsRISCV::R_RISCV_HI20).first == RelocationCode::UnsupportedType);
  CHECK(apply(EM_NONE, R_PEP10_ABS16, 0, 0, 0).first == RelocationCode::UnsupportedType);
}
