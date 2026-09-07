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
#include <optional>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>
#include "core/formats/elf/enums.hpp"
#include "core/formats/elf/managed_types.hpp"

namespace pepp::bts {

// A section which occpies no file space but still occupies loaded memory, like .bss
struct NoBits {
  uxword size = 0;
};

// A section whose payload is opaque to the ELF library, like .text or .data
// Memory size will be assumed to be equal to file size, equal to vector's length.
struct RawBytes {
  std::vector<u8> bytes;
};

/*
 * Base for (include-heavy) section payloads, such as string tables, symbol tables, and relocations. Unfortunately,
 * std::visit over SectionData is no longer exhaustive, but I think this is the less evil than pulling in a large number
 * of headers.
 */
struct ManagedPayload {
  virtual ~ManagedPayload();
  // Bytes this payload contributes to the file for a given bitness
  virtual uxword file_bytes(ElfBits bits) const = 0;
  // Some sections (e.g., hash tables) require other sections be serialized first. These dependencies must be kept alive
  // during GC as well.
  virtual void collect_dependencies(std::vector<SectionRef> &out) const {}
};

/*
 * Payload data for a section. Monostate can be used for anything with no data + no file size, like SHT_NULL.
 */
using SectionData = std::variant<std::monostate, NoBits, RawBytes, std::unique_ptr<ManagedPayload>>;

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
  ManagedSection(std::string name, SectionTypes type, ElfBits bits) : type(type), name(std::move(name)), _bits(bits) {}

  SectionTypes type = SectionTypes::SHT_NULL;
  u32 addralign = 0;
  SectionFlags flags = {};
  uxword addr = 0;

  SectionRef link = SectionRef{0};
  std::string name;

  SectionData content;

  // Type of sh_info depends on the section. If it's being interpreted as a section index, please use the SectionRef
  // alternative. Otherwise, use the u32 as the spec requires.
  std::variant<u32, SectionRef> info = u32{0};

  // If this member is set, then this section must be assigned a specific index in the final ELF file.
  // Mostly used to identify SHN_ABS and SHN_COMMON.
  std::optional<SectionIndices> required_index = std::nullopt;
  // The word size of the file this section belongs to.
  ElfBits bits() const noexcept { return _bits; }
  // Bytes this section directly contributes to the final object file, not counting inter-section alignment/padding.
  uxword file_bytes() const;
  // Bytes this section occupies when loaded into memory, which differs from file_bytes() for a NoBits section.
  uxword memory_bytes() const;
  uxword sh_size() const;

  // Replace the section's content with a ManagedPayload subclass.
  template <class T, class... Args> T &make_content(Args &&...args) {
    static_assert(std::is_base_of_v<ManagedPayload, T>, "must derive from ManagedPayload");
    auto &held = content.emplace<std::unique_ptr<ManagedPayload>>(std::make_unique<T>(std::forward<Args>(args)...));
    return static_cast<T &>(*held);
  }
  // Try to cast to the correct ManagedPayload subclass.
  // Would prefer to use deducing this, but as of 2026-09-07, GCC in CI doesn't support it.
  template <class T> T *content_as() noexcept {
    static_assert(std::is_base_of_v<ManagedPayload, T>, "must derive from ManagedPayload");
    auto *held = std::get_if<std::unique_ptr<ManagedPayload>>(&content);
    return held ? dynamic_cast<T *>(held->get()) : nullptr;
  }
  template <class T> const T *content_as() const noexcept {
    static_assert(std::is_base_of_v<ManagedPayload, T>, "must derive from ManagedPayload");
    const auto *held = std::get_if<std::unique_ptr<ManagedPayload>>(&content);
    return held ? dynamic_cast<const T *>(held->get()) : nullptr;
  }

  bool is_serialized() const noexcept {
    using SI = SectionIndices;
    if (!required_index.has_value()) return true;
    else if (auto v = *required_index; v == SI::SHN_ABS || v == SI::SHN_COMMON) return false;
    else return true;
  }

private:
  ElfBits _bits;
};

} // namespace pepp::bts
