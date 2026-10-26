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
#include <expected>
#include <filesystem>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>
#include "core/ds/opaque_handle.hpp"
#include "core/formats/ar/packed_archive.hpp"
#include "core/formats/elf/packed_elf.hpp"
#include "core/math/geom/interval.hpp"

namespace pepp::bts {
// Identifies one file within a PackedInputElfGroup. IDs are dense from 1, so a default-constructed ID is invalid.
using ElfFileID = pepp::OpaqueHandle<struct ElfFileTag, u32>;

// Identifies a section or segment within a PackedInputElfGroup by its file and its index within that file.
// Files are only ever appended to a group, so a handle stays valid for the group's lifetime.
template <class Tag> struct InputElfItemHandle {
  ElfFileID file{};
  u32 index = 0;

  auto operator<=>(const InputElfItemHandle &) const = default;
  bool operator==(const InputElfItemHandle &) const = default;
  // Index 0 is a real item (e.g., the null section), so only the file decides validity.
  constexpr explicit operator bool() const { return bool(file); }
};
using InputElfSectionHandle = InputElfItemHandle<struct InputElfSectionTag>;
using InputElfSegmentHandle = InputElfItemHandle<struct InputElfSegmentTag>;

// Identifies one archive within a PackedInputElfGroup. IDs are dense from 1. A default-constructed ID is invalid.
using ElfArchiveID = pepp::OpaqueHandle<struct ElfArchiveTag, u32>;

struct InputGroupError {
  enum class Code : u8 {
    CannotOpen,
    // Neither ELF nor archive magic.
    UnknownFormat,
    BadElf,
    // The file's class, byte order, or machine differs from the group's.
    WrongClassOrByteOrder,
    WrongMachine,
    BadArchive,
    NoSuchArchive,
    TooManyInputs,
  };
  static std::string_view to_string(Code code);

  Code code;
  // The underlying error, when it came from an archive or one of its members.
  std::optional<ArchiveError> archive = std::nullopt;
};

// Where an ELF file in a group came from.
struct ElfFromPath {
  std::filesystem::path path;
};
struct ElfFromArchive {
  ElfArchiveID archive;
  PackedArMember member;
};
struct ElfInMemory {};
using ElfProvenance = std::variant<ElfInMemory, ElfFromPath, ElfFromArchive>;

namespace detail {
struct DetectedInput {
  enum class Format : u8 { Elf, Archive } format = Format::Elf;
  std::shared_ptr<const AElfSource> source;
};
// Maps path and identifies its format by its magic.
std::expected<DetectedInput, InputGroupError> detect_input(const std::filesystem::path &path);
std::expected<AnyInputElf, InputGroupError> open_elf(std::shared_ptr<const AElfSource> source);
std::expected<AnyInputElf, InputGroupError> open_member(const PackedInputArchive &archive,
                                                        const PackedArMember &member);
} // namespace detail

// A group of read-only ELF files which share a word size and byte order, such as every input to a single load.
// Handles name sections and segments across the whole group, so callers need not track which file each came from.
template <ElfBits B, ElfEndian E> class PackedInputElfGroup {
public:
  using File = PackedInputElfFile<B, E>;
  using Shdr = PackedElfShdr<B, E>;
  using Phdr = PackedElfPhdr<B, E>;

  using Input = std::variant<ElfFileID, ElfArchiveID>;
  template <class T> using Result = std::expected<T, InputGroupError>;

  // Takes ownership of elf, returning the ID that its handles will use.
  // Fails if its class, byte order, or machine differs from this group's.
  Result<ElfFileID> add(AnyInputElf elf);
  // Adds an archive to the group, whose members cant be extracted manually via add_member.
  Result<ElfArchiveID> add(PackedInputArchive archive);
  // Open a file or archive as determined by its file magic.
  Result<Input> add(const std::filesystem::path &path);
  // Adds one member of an archive in this group as an ELF file.
  Result<ElfFileID> add_member(ElfArchiveID id, const PackedArMember &member);

  std::size_t size() const noexcept { return _files.size(); }
  std::size_t archive_count() const noexcept { return _archives.size(); }
  const File &file(ElfFileID id) const;
  const ElfProvenance &provenance(ElfFileID id) const;
  // Throws std::out_of_range if no archive has this ID.
  const PackedInputArchive &archive(ElfArchiveID id) const;
  // Handles for every section / segment of a file in index order, including the null section at index 0.
  std::vector<InputElfSectionHandle> sections(ElfFileID id) const;
  std::vector<InputElfSegmentHandle> segments(ElfFileID id) const;

  // All lookups throw std::out_of_range if the handle does not name an item in this group.
  const Shdr &header(InputElfSectionHandle handle) const;
  const Phdr &header(InputElfSegmentHandle handle) const;
  std::shared_ptr<const AStorage> data(InputElfSectionHandle handle) const;
  // The contiguous file bytes of a segment. 0-padding from p_filesz to p_memsz is not included.
  std::shared_ptr<const AStorage> data(InputElfSegmentHandle handle) const;

private:
  struct Entry {
    std::unique_ptr<File> file;
    ElfProvenance provenance;
  };
  Result<ElfFileID> insert(AnyInputElf elf, ElfProvenance provenance);
  Result<ElfArchiveID> insert(PackedInputArchive archive);
  static std::unexpected<InputGroupError> fail(InputGroupError::Code code) {
    return std::unexpected(InputGroupError{code});
  }

  std::vector<Entry> _files;
  // Must ensure pointer stability for archives on insert to the vector.
  std::vector<std::unique_ptr<PackedInputArchive>> _archives;
};

// Helper to wrap wrap one file in a group of the same type
AnyElfGroup to_input_group(AnyInputElf elf);

template <ElfBits B, ElfEndian E> auto PackedInputElfGroup<B, E>::add(AnyInputElf elf) -> Result<ElfFileID> {
  return insert(std::move(elf), ElfInMemory{});
}

template <ElfBits B, ElfEndian E>
auto PackedInputElfGroup<B, E>::add(PackedInputArchive archive) -> Result<ElfArchiveID> {
  return insert(std::move(archive));
}

template <ElfBits B, ElfEndian E>
auto PackedInputElfGroup<B, E>::add(const std::filesystem::path &path) -> Result<Input> {
  auto detected = detail::detect_input(path);
  if (!detected) return std::unexpected(detected.error());
  else if (detected->format == detail::DetectedInput::Format::Archive) {
    auto archive = PackedInputArchive::open(detected->source, path);
    if (!archive) return std::unexpected(InputGroupError{InputGroupError::Code::BadArchive, archive.error()});
    return insert(std::move(*archive));
  }
  auto elf = detail::open_elf(detected->source);
  if (!elf) return std::unexpected(elf.error());
  return insert(std::move(*elf), ElfFromPath{path});
}

template <ElfBits B, ElfEndian E>
auto PackedInputElfGroup<B, E>::add_member(ElfArchiveID id, const PackedArMember &member) -> Result<ElfFileID> {
  if (!id || id.value > _archives.size()) return fail(InputGroupError::Code::NoSuchArchive);
  auto elf = detail::open_member(*_archives[id.value - 1], member);
  if (!elf) return std::unexpected(elf.error());
  return insert(std::move(*elf), ElfFromArchive{id, member});
}

template <ElfBits B, ElfEndian E>
auto PackedInputElfGroup<B, E>::insert(AnyInputElf elf, ElfProvenance provenance) -> Result<ElfFileID> {
  using enum InputGroupError::Code;
  auto *held = std::get_if<std::unique_ptr<File>>(&elf);
  if (held == nullptr) return fail(WrongClassOrByteOrder);
  else if (*held == nullptr) throw std::invalid_argument("PackedInputElfGroup::add: file must be non-null");
  else if (!_files.empty() && u16((*held)->header.e_machine) != u16(_files.front().file->header.e_machine))
    return fail(WrongMachine);
  else if (_files.size() >= std::numeric_limits<ElfFileID::underlying_type>::max()) return fail(TooManyInputs);
  _files.push_back({std::move(*held), std::move(provenance)});
  return ElfFileID(static_cast<ElfFileID::underlying_type>(_files.size()));
}

template <ElfBits B, ElfEndian E>
auto PackedInputElfGroup<B, E>::insert(PackedInputArchive archive) -> Result<ElfArchiveID> {
  if (_archives.size() >= std::numeric_limits<ElfArchiveID::underlying_type>::max())
    return fail(InputGroupError::Code::TooManyInputs);
  _archives.push_back(std::make_unique<PackedInputArchive>(std::move(archive)));
  return ElfArchiveID(static_cast<ElfArchiveID::underlying_type>(_archives.size()));
}

template <ElfBits B, ElfEndian E> auto PackedInputElfGroup<B, E>::file(ElfFileID id) const -> const File & {
  if (!id || id.value > _files.size()) throw std::out_of_range("PackedInputElfGroup: no file with this ID");
  return *_files[id.value - 1].file;
}

template <ElfBits B, ElfEndian E>
auto PackedInputElfGroup<B, E>::provenance(ElfFileID id) const -> const ElfProvenance & {
  if (!id || id.value > _files.size()) throw std::out_of_range("PackedInputElfGroup: no file with this ID");
  return _files[id.value - 1].provenance;
}

template <ElfBits B, ElfEndian E>
auto PackedInputElfGroup<B, E>::archive(ElfArchiveID id) const -> const PackedInputArchive & {
  if (!id || id.value > _archives.size()) throw std::out_of_range("PackedInputElfGroup: no archive with this ID");
  return *_archives[id.value - 1];
}

template <ElfBits B, ElfEndian E>
std::vector<InputElfSectionHandle> PackedInputElfGroup<B, E>::sections(ElfFileID id) const {
  const auto count = file(id).section_headers.size();
  std::vector<InputElfSectionHandle> ret(count);
  for (std::size_t it = 0; it < count; ++it) ret[it] = InputElfSectionHandle{id, static_cast<u32>(it)};
  return ret;
}

template <ElfBits B, ElfEndian E>
std::vector<InputElfSegmentHandle> PackedInputElfGroup<B, E>::segments(ElfFileID id) const {
  const auto count = file(id).program_headers.size();
  std::vector<InputElfSegmentHandle> ret(count);
  for (std::size_t it = 0; it < count; ++it) ret[it] = InputElfSegmentHandle{id, static_cast<u32>(it)};
  return ret;
}

template <ElfBits B, ElfEndian E>
auto PackedInputElfGroup<B, E>::header(InputElfSectionHandle handle) const -> const Shdr & {
  const auto &f = file(handle.file);
  if (handle.index >= f.section_headers.size()) throw std::out_of_range("PackedInputElfGroup: no such section");
  return f.section_headers[handle.index];
}

template <ElfBits B, ElfEndian E>
auto PackedInputElfGroup<B, E>::header(InputElfSegmentHandle handle) const -> const Phdr & {
  const auto &f = file(handle.file);
  if (handle.index >= f.program_headers.size()) throw std::out_of_range("PackedInputElfGroup: no such segment");
  return f.program_headers[handle.index];
}

template <ElfBits B, ElfEndian E>
std::shared_ptr<const AStorage> PackedInputElfGroup<B, E>::data(InputElfSectionHandle handle) const {
  const auto &f = file(handle.file);
  if (handle.index >= f.section_data.size()) throw std::out_of_range("PackedInputElfGroup: no such section");
  return f.section_data[handle.index];
}

template <ElfBits B, ElfEndian E>
std::shared_ptr<const AStorage> PackedInputElfGroup<B, E>::data(InputElfSegmentHandle handle) const {
  const auto &f = file(handle.file);
  if (handle.index >= f.program_headers.size()) throw std::out_of_range("PackedInputElfGroup: no such segment");
  return f.segment_data(handle.index);
}

// Extract the PT_LOAD-able segments from the group, returning their virtual memory address spans and segment handles.
template <ElfBits B, ElfEndian E>
std::vector<std::pair<InputElfSegmentHandle, pepp::core::Interval<word<B>>>>
loadable_segments(const PackedInputElfGroup<B, E> &group) {
  std::vector<std::pair<InputElfSegmentHandle, pepp::core::Interval<word<B>>>> ret;
  for (std::size_t id = 1; id <= group.size(); ++id) {
    for (const auto handle : group.segments(ElfFileID(static_cast<ElfFileID::underlying_type>(id)))) {
      const auto &phdr = group.header(handle);
      if (phdr.p_type != bits::to_underlying(SegmentType::PT_LOAD) || phdr.p_memsz == 0) continue;
      const word<B> start = phdr.p_vaddr, size = phdr.p_memsz;
      if (size - 1 > std::numeric_limits<word<B>>::max() - start)
        throw std::runtime_error("loadable_segments: segment extends past the end of the address space");
      ret.emplace_back(handle, pepp::core::Interval<word<B>>(start, start + (size - 1)));
    }
  }
  return ret;
}

} // namespace pepp::bts
