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
#include "core/formats/elf/enums_sections.hpp"
#include "core/math/bitmanip/enums.hpp"

namespace {
std::optional<u8> pep_relocation_size(u32 type) {
  using enum pepp::bts::RelocationsPep;
  switch (static_cast<pepp::bts::RelocationsPep>(type)) {
  case R_PEP10_NONE: return 0;
  case R_PEP10_ABS8: return 1;
  case R_PEP10_ABS16: return 2;
  case R_PEP10_ABS32: return 4;
  default: return std::nullopt;
  }
}
// TODO: the rest of the RISC-V psABI's relocations.
std::optional<u8> riscv_relocation_size(u32 type) {
  using enum pepp::bts::RelocationsRISCV;
  switch (static_cast<pepp::bts::RelocationsRISCV>(type)) {
  case R_RISCV_NONE: return 0;
  case R_RISCV_32: return 4;
  case R_RISCV_64: return 8;
  default: return std::nullopt;
  }
}
} // namespace

std::optional<u8> pepp::link::relocation_size(bts::ElfMachineType machine, u32 type) {
  using enum bts::ElfMachineType;
  if (machine == EM_PEP8 || machine == EM_PEP9 || machine == EM_PEP10) return pep_relocation_size(type);
  else if (machine == EM_RISCV) return riscv_relocation_size(type);
  return std::nullopt;
}
