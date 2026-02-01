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
#include <memory>
#include <variant>
#include "../../integers.h"
#include "core/formats/elf/packed_types.hpp"

namespace pepp::bts {
struct AStorage;
template <ElfBits, ElfEndian> class PackedElf;

// Sometime you want to operate on _some_ ELF file, but you can't be templatized based on ElfBits and ElfEndian.
// e.g., you are a subclass with a virtual function, but you want to operate on an ELF file passed in at runtime.
using PackedElfLE32 = PackedElf<ElfBits::b32, ElfEndian::le>;
using PackedElfBE32 = PackedElf<ElfBits::b32, ElfEndian::be>;
using PackedElfLE64 = PackedElf<ElfBits::b64, ElfEndian::le>;
using PackedElfBE64 = PackedElf<ElfBits::b64, ElfEndian::be>;
using AnyPackedElfPtr = std::variant<PackedElfLE32 *, PackedElfBE32 *, PackedElfLE64 *, PackedElfBE64 *>;
using ConstAnyPackedElfPtr =
    std::variant<const PackedElfLE32 *, const PackedElfBE32 *, const PackedElfLE64 *, const PackedElfBE64 *>;
std::shared_ptr<const pepp::bts::AStorage> section_data(ConstAnyPackedElfPtr elf, u16 section_index);
u32 sh_align(ConstAnyPackedElfPtr elf, u16 section_index);

} // namespace pepp::core
