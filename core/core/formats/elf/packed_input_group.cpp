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

#include "packed_input_group.hpp"
#include <system_error>
#include "core/formats/elf/packed_io.hpp"
#include "core/io/mapped_file.hpp"
#include "core/math/bitmanip/strings.hpp"

std::string_view pepp::bts::InputGroupError::to_string(Code code) {
  switch (code) {
  case Code::CannotOpen: return "cannot open file";
  case Code::UnknownFormat: return "not an ELF file or archive";
  case Code::BadElf: return "malformed ELF file";
  case Code::WrongClassOrByteOrder: return "ELF class or byte order differs from the group's";
  case Code::WrongMachine: return "ELF machine differs from the group's";
  case Code::BadArchive: return "malformed archive";
  case Code::NoSuchArchive: return "no archive with this ID";
  case Code::TooManyInputs: return "too many inputs";
  }
  return "unknown error";
}

auto pepp::bts::detail::detect_input(const std::filesystem::path &path)
    -> std::expected<DetectedInput, InputGroupError> {
  using enum InputGroupError::Code;
  auto source = MappedFileSource::open(path);
  if (!source) return std::unexpected(InputGroupError{CannotOpen});
  DetectedInput ret{.source = std::move(source)};
  const auto slice = ret.source->slice(0, PackedInputArchive::REGULAR_MAGIC.size());
  const auto bytes = slice->get(0, slice->size());
  const auto magic = bits::as_string_view(bytes);
  if (magic == PackedInputArchive::REGULAR_MAGIC || magic == PackedInputArchive::THIN_MAGIC)
    ret.format = DetectedInput::Format::Archive;
  else if (!has_elf_magic(bytes)) return std::unexpected(InputGroupError{UnknownFormat});
  return ret;
}

auto pepp::bts::detail::open_elf(std::shared_ptr<const AElfSource> source)
    -> std::expected<AnyInputElf, InputGroupError> {
  auto elf = try_open_input_elf(std::move(source));
  if (!elf) return std::unexpected(InputGroupError{InputGroupError::Code::BadElf});
  return std::move(*elf);
}

auto pepp::bts::detail::open_member(const PackedInputArchive &archive, const PackedArMember &member)
    -> std::expected<AnyInputElf, InputGroupError> {
  using enum InputGroupError::Code;
  auto elf = open_member_elf(archive, member);
  if (elf) return std::move(*elf);
  // A member that cannot be read leaves the archive itself intact.
  InputGroupError ret{BadArchive, elf.error()};
  if (elf.error().code == ArchiveError::Code::NotElf) ret.code = BadElf;
  else if (elf.error().code == ArchiveError::Code::CannotOpen) ret.code = CannotOpen;
  return std::unexpected(ret);
}

auto pepp::bts::open_input(const std::filesystem::path &path) -> std::expected<OpenedInput, InputGroupError> {
  auto detected = detail::detect_input(path);
  if (!detected) return std::unexpected(detected.error());
  else if (detected->format == detail::DetectedInput::Format::Archive) {
    auto archive = PackedInputArchive::open(detected->source, path);
    if (!archive) return std::unexpected(InputGroupError{InputGroupError::Code::BadArchive, archive.error()});
    return std::move(*archive);
  }
  auto elf = detail::open_elf(detected->source);
  if (!elf) return std::unexpected(elf.error());
  return std::move(*elf);
}

pepp::bts::AnyElfGroup pepp::bts::to_input_group(AnyInputElf elf) {
  auto visitor = [](auto &file) -> AnyElfGroup {
    using File = typename std::remove_reference_t<decltype(file)>::element_type;
    auto group = std::make_unique<PackedInputElfGroup<File::elf_bits, File::elf_endian>>();
    if (auto id = group->add(std::move(file)); !id)
      throw std::runtime_error(std::string(InputGroupError::to_string(id.error().code)));
    return group;
  };
  return std::visit(visitor, elf);
}
