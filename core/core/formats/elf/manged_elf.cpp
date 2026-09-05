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

#include "manged_elf.hpp"
#include <cassert>
#include "core/formats/elf/managed_section.hpp"

pepp::bts::ManagedElf::ManagedElf(ElfBits bits, ElfEndian endian, ElfFileType type, ElfMachineType machine, ElfABI abi)
    : type(type), machine(machine), abi(abi), _bits(bits), _endian(endian) {
  // I want to keep the "magic" section refs as compile-time constants.
  // Assert no drift between those constants and what has actually been assigned.
  _sections.emplace_back(nullptr);
  const auto r0 = SectionRef{0};
  assert(r0 == ManagedElf::SHN_UNDEF);

  auto add_pseudo = [this](std::string name, SectionIndices index) {
    auto sec = std::make_unique<ManagedSection>(std::move(name), SectionTypes::SHT_NULL);
    sec->required_index = index;
    _sections.push_back(std::move(sec));
    return SectionRef{static_cast<SectionRef::underlying_type>(_sections.size() - 1)};
  };
  const auto r1 = add_pseudo("SHN_ABS", SectionIndices::SHN_ABS);
  assert(r1 == ManagedElf::SHN_ABS);
  const auto r2 = add_pseudo("SHN_COMMON", SectionIndices::SHN_COMMON);
  assert(r2 == ManagedElf::SHN_COMMON);
}

pepp::bts::ManagedElf::~ManagedElf() = default;
pepp::bts::ManagedElf::ManagedElf(ManagedElf &&) noexcept = default;
pepp::bts::ManagedElf &pepp::bts::ManagedElf::operator=(ManagedElf &&) noexcept = default;

pepp::bts::SectionRef pepp::bts::ManagedElf::add_section(std::string name, SectionTypes type) {
  _sections.push_back(std::make_unique<ManagedSection>(std::move(name), type));
  return SectionRef{static_cast<SectionRef::underlying_type>(_sections.size() - 1)};
}

pepp::bts::ManagedSection *pepp::bts::ManagedElf::section(SectionRef ref) noexcept {
  if (!ref || ref.value >= _sections.size()) return nullptr;
  return _sections[ref.value].get();
}

const pepp::bts::ManagedSection *pepp::bts::ManagedElf::section(SectionRef ref) const noexcept {
  if (!ref || ref.value >= _sections.size()) return nullptr;
  return _sections[ref.value].get();
}
