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

#pragma once
#include <optional>
#include <span>
#include "core/formats/elf/enums_eheader.hpp"
#include "core/formats/elf/packed_types.hpp"
#include "core/integers.h"
#include "core/link/diagnostics.hpp"
#include "core/math/bitmanip/copy.hpp"
#include "core/math/bitmanip/log2.hpp"

namespace pepp::link {

// Returns the number of bytes written by a relocation that computes S + A, or 0 for a relocation that writes nothing.
// Returns nullopt for every other type.
std::optional<u8> relocation_size(bts::ElfMachineType machine, u32 type);

// Only works for relocations which overwrite target bytes. relocations which read-modify-write (RV instructions) are
// unsupported.
template <bts::ElfBits B, bts::ElfEndian E>
std::optional<RelocationCode> apply_relocation(bts::ElfMachineType machine, const bts::PackedElfRelA<B, E> &rela,
                                               u64 symbol, std::span<u8> section) {
  const auto size = relocation_size(machine, rela.r_type());
  if (!size) return RelocationCode::UnsupportedType;
  else if (*size == 0) return std::nullopt;
  const u64 offset = rela.r_offset;
  if (offset > section.size() || *size > section.size() - offset) return RelocationCode::BadOffset;
  const u64 value = symbol + static_cast<u64>(static_cast<i64>(bts::sword<B>(rela.r_addend)));
  if (!bits::int_fits(value, *size)) return RelocationCode::Truncated;
  const auto order = E == bts::ElfEndian::be ? bits::Order::BigEndian : bits::Order::LittleEndian;
  bits::memcpy_endian(section.subspan(offset, *size), order, value);
  return std::nullopt;
}
} // namespace pepp::link
