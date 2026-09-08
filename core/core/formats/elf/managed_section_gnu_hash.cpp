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

#include "core/formats/elf/managed_section_gnu_hash.hpp"
#include "core/formats/elf/packed_types.hpp"

pepp::bts::ManagedGnuHash::ManagedGnuHash(SectionRef symtab, SectionRef strtab, GnuHashParameters parameters)
    : _symtab(symtab), _strtab(strtab), _parameters(parameters) {}

pepp::bts::uxword pepp::bts::ManagedGnuHash::file_bytes(ElfBits bits) const {
  // Four u32 of header, then the bloom filter in target words, then a u32 per bucket and per hashed symbol.
  // See packed_access_hash
  const uxword entries = _parameters.nbuckets + _parameters.hashed_count;
  return 4u * sizeof(u32) + _parameters.maskwords * word_bytes(bits) + entries * sizeof(u32);
}

void pepp::bts::ManagedGnuHash::collect_dependencies(std::vector<SectionRef> &out) const {
  out.push_back(_symtab), out.push_back(_strtab);
}
