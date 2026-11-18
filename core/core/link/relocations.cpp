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
#include "core/math/bitmanip/copy.hpp"
#include "core/math/bitmanip/enums.hpp"
#include "core/math/bitmanip/log2.hpp"

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

// Returns the number of bytes written by a relocation that computes S + A, or 0 for a relocation that writes nothing.
// Returns nullopt for every other type.
std::optional<u8> relocation_size(pepp::bts::ElfMachineType machine, u32 type) {
  using enum pepp::bts::ElfMachineType;
  if (machine == EM_PEP8 || machine == EM_PEP9 || machine == EM_PEP10) return pep_relocation_size(type);
  else if (machine == EM_RISCV) return riscv_relocation_size(type);
  return std::nullopt;
}
} // namespace

// Only works for relocations which overwrite target bytes. Relocations which read-modify-write (RV instructions) are
// unsupported.
std::optional<pepp::link::RelocationCode> pepp::link::apply_relocation(bts::ElfMachineType machine,
                                                                       bts::ElfEndian endian,
                                                                       const ResolvedRelocation &rel,
                                                                       std::span<u8> section) {
  using enum RelocationCode;
  const auto order = endian == bts::ElfEndian::be ? bits::Order::BigEndian : bits::Order::LittleEndian;
  const auto size = relocation_size(machine, rel.type);
  if (!rel.symbol) return NoSymbolAddress;
  else if (!size) return UnsupportedType;
  else if (*size == 0) return std::nullopt;
  else if (rel.offset > section.size() || *size > section.size() - rel.offset) return BadOffset;
  const u64 value = *rel.symbol + static_cast<u64>(rel.addend);
  if (!bits::int_fits(value, *size)) return Truncated;
  bits::memcpy_endian(section.subspan(rel.offset, *size), order, value);
  return std::nullopt;
}
