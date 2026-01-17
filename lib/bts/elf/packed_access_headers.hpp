#pragma once
#include "bts/bitmanip/integers.h"
#include "bts/elf/packed_types.hpp"

namespace pepp::bts {
struct AStorage;
template <ElfBits, ElfEndian> struct PackedElf;

using PackedElfLE32 = PackedElf<ElfBits::b32, ElfEndian::le>;
using PackedElfBE32 = PackedElf<ElfBits::b32, ElfEndian::be>;
using PackedElfLE64 = PackedElf<ElfBits::b64, ElfEndian::le>;
using PackedElfBE64 = PackedElf<ElfBits::b64, ElfEndian::be>;
using AnyPackedElfPtr = std::variant<PackedElfLE32 *, PackedElfBE32 *, PackedElfLE64 *, PackedElfBE64 *>;
using ConstAnyPackedElfPtr =
    std::variant<const PackedElfLE32 *, const PackedElfBE32 *, const PackedElfLE64 *, const PackedElfBE64 *>;
std::shared_ptr<const pepp::bts::AStorage> section_data(ConstAnyPackedElfPtr elf, u16 section_index);
u32 sh_align(ConstAnyPackedElfPtr elf, u16 section_index);

} // namespace pepp::bts
