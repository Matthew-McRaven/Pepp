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
#include <variant>
#include "core/formats/elf/enums_eheader.hpp"
#include "core/formats/elf/packed_types.hpp"
#include "core/integers.h"
#include "core/link/diagnostics.hpp"

namespace pepp::link {

// A relocation from the input with its symbol resolved to one in the output.
struct ResolvedRelocation {
  ResolvedRelocation(u64 offset, u32 type, std::optional<u64> symbol, i64 addend)
      : offset(offset), type(type), symbol(symbol), addend(addend) {}
  template <bts::ElfBits B, bts::ElfEndian E>
  ResolvedRelocation(const bts::PackedElfRelA<B, E> &rela, std::optional<u64> symbol)
      : offset(rela.r_offset), type(static_cast<u32>(rela.r_type())), symbol(symbol),
        addend(static_cast<i64>(bts::sword<B>(rela.r_addend))) {}

  u64 offset;                // Of the relocated field within its section.
  u32 type;                  // machine-dependent relocation type.
  std::optional<u64> symbol; // S. Empty if the definition is not placed in the output.
  i64 addend;                // A
};

struct PairedRelocation {
  // The relocation currently being applied.
  ResolvedRelocation rel;
  // The additional relocation entry consumed by rel.
  ResolvedRelocation pair;
};

// There exists a relocation with this (offset, type) in the current setion
struct RelocationKey {
  u64 offset = 0;
  u32 type = 0;
};
// Given a relocation which is the second member of a pair, find the first member of the pair, if it exists.
std::optional<RelocationKey> paired_key(bts::ElfMachineType machine, u64 address, const ResolvedRelocation &rel);

std::optional<RelocationCode> apply_relocation(bts::ElfMachineType machine, bts::ElfBits width, bts::ElfEndian endian,
                                               u64 address, std::span<u8> bytes,
                                               const std::variant<ResolvedRelocation, PairedRelocation> &relocation);

// Apply a relocation to bytes, the output's copy of its section placed at address. Returns the reason if it cannot be
// applied.
template <bts::ElfBits B, bts::ElfEndian E>
std::optional<RelocationCode> apply_relocation(bts::ElfMachineType machine, u64 address, std::span<u8> bytes,
                                               const std::variant<ResolvedRelocation, PairedRelocation> &relocation) {
  return apply_relocation(machine, B, E, address, bytes, relocation);
}
} // namespace pepp::link
