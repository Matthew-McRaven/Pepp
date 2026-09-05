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
#include <string>
#include <variant>
#include "core/formats/elf/enums.hpp"
#include "core/formats/elf/managed_types.hpp"

namespace pepp::bts {

/*
 * A single section of a ManagedElf file. Common fields are stored in this class, while content is stored in a variant.
 *
 * As with the file header, only authored fields are stored, so the following fields are not stored:
 *   sh_offset            assigned during layout
 *   sh_size              the serialized size of `content`
 *   sh_entsize           depends only on the content kind and the file's ElfBits
 *   sh_name              assigned when .shstrtab is filled
 */
class ManagedSection {
public:
  ManagedSection() = default;
  ManagedSection(std::string name, SectionTypes type) : name(std::move(name)), type(type) {}

  SectionTypes type = SectionTypes::SHT_NULL;
  u32 addralign = 0;
  SectionFlags flags = {};
  uxword addr = 0;

  SectionRef link = SectionRef{0};
  std::string name;

  // TODO: variant<StringTable, SymbolTable, RelocTable, NoteTable, RawBytes, NoBits>, one alternative
  // per section type we support. Use RawBytes for unknown section types.
  std::variant<std::monostate> content;

  // Type of sh_info depends on the section. If it's being interpreted as a section index, please use the SectionRef
  // alternative. Otherwise, use the u32 as the spec requires.
  std::variant<u32, SectionRef> info = u32{0};

  // If this member is set, then this section must be assigned a specific index in the final ELF file.
  // Mostly used to identify SHN_ABS and SHN_COMMON.
  std::optional<SectionIndices> required_index = std::nullopt;
  bool is_serialized() const noexcept {
    using SI = SectionIndices;
    if (!required_index.has_value()) return true;
    else if (auto v = *required_index; v == SI::SHN_ABS || v == SI::SHN_COMMON) return false;
    else return true;
  }
};

} // namespace pepp::bts
