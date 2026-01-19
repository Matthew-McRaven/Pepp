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

#include "core/elf/packed_types.hpp"

void pepp::bts::detail::fill_e_ident(std::span<u8, 16> e_ident, ElfBits B, ElfEndian E, ElfABI abi,
                                     u8 abi_version) noexcept {
  e_ident[to_underlying(ElfIdentifierIndices::EI_MAG0)] = to_underlying(ElfMagic::ELFMAG0);
  e_ident[to_underlying(ElfIdentifierIndices::EI_MAG1)] = to_underlying(ElfMagic::ELFMAG1);
  e_ident[to_underlying(ElfIdentifierIndices::EI_MAG2)] = to_underlying(ElfMagic::ELFMAG2);
  e_ident[to_underlying(ElfIdentifierIndices::EI_MAG3)] = to_underlying(ElfMagic::ELFMAG3);
  e_ident[to_underlying(ElfIdentifierIndices::EI_CLASS)] =
      to_underlying(B == ElfBits::b64 ? ElfClass::ELFCLASS64 : ElfClass::ELFCLASS32);
  e_ident[to_underlying(ElfIdentifierIndices::EI_DATA)] =
      to_underlying(E == ElfEndian::le ? ElfEncoding::ELFDATA2LSB : ElfEncoding::ELFDATA2MSB);
  e_ident[to_underlying(ElfIdentifierIndices::EI_VERSION)] = to_underlying(ElfVersion::EV_CURRENT);
  e_ident[to_underlying(ElfIdentifierIndices::EI_OSABI)] = to_underlying(abi);
  e_ident[to_underlying(ElfIdentifierIndices::EI_ABIVERSION)] = abi_version;
  auto padding = e_ident.subspan(to_underlying(ElfIdentifierIndices::EI_PAD));
  memset(padding.data(), 0, padding.size());
}
