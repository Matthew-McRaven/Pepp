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
#include "core/link/pass_populate_group.hpp"
#include <filesystem>
#include <memory>
#include <type_traits>
#include <utility>
#include <variant>

namespace {
using namespace pepp;
using namespace pepp::link;
using bts::InputGroupError;

// Report a Fatal error for input, preferring the archive's code and offset because they are more specific than the
// group's. Returns false to simplify populate().
bool bad_input(Diagnostics &diagnostics, ConfigInputID input, const InputGroupError &error) {
  if (error.archive)
    diagnostics.add(
        {Severity::Fatal, error.archive->code, ConfigInputOffset{input, static_cast<u32>(error.archive->offset)}});
  else diagnostics.add({Severity::Fatal, error.code, input});
  return false;
}

std::optional<bts::AnyElfGroup> group_type_of(const bts::AnyInputElf &elf) {
  auto f = [](const auto &file) -> bts::AnyElfGroup {
    using File = typename std::remove_cvref_t<decltype(file)>::element_type;
    return std::make_unique<bts::PackedInputElfGroup<File::elf_bits, File::elf_endian>>();
  };
  return std::visit(f, elf);
}
std::optional<bts::AnyElfGroup> group_type_of(const bts::PackedInputArchive &archive) {
  for (const auto &member : archive.members()) {
    if (!member) break;
    else if (const auto elf = bts::open_member_elf(archive, *member)) return group_type_of(*elf);
  }
  return std::nullopt;
}
std::optional<bts::AnyElfGroup> group_type_of(const LinkInput::Script &) { return std::nullopt; }
// parse_scripts() already opened every path it could.
std::optional<bts::AnyElfGroup> group_type_of(const std::filesystem::path &) { return std::nullopt; }
std::optional<bts::AnyElfGroup> group_type_of(const LinkInput::Opened &opened) {
  return std::visit([](const auto &file) { return group_type_of(file); }, opened.file);
}

// Add every input to group and set the liveness of each file. Returns false after a Fatal error.
template <bts::ElfBits B, bts::ElfEndian E>
bool populate(Diagnostics &diagnostics, bts::PackedInputElfGroup<B, E> &group, std::vector<LinkInput> inputs,
              std::vector<Liveness> &files) {
  // Eagerly extract archive members (like ELF). Later reachability passes remove unused members from the output.
  const auto add_members = [&](bts::ElfArchiveID archive, ConfigInputID input) {
    for (const auto &member : group.archive(archive).members()) {
      if (!member) return bad_input(diagnostics, input, {InputGroupError::Code::BadArchive, member.error()});
      else if (const auto id = group.add_member(archive, *member); !id)
        return bad_input(diagnostics, input, id.error());
    }
    return true;
  };
  const auto add_archive = [&](bts::PackedInputArchive archive, ConfigInputID input) {
    const auto id = group.add(std::move(archive));
    if (!id) return bad_input(diagnostics, input, id.error());
    return add_members(*id, input);
  };
  // Indexed by ConfigInputID. Each input adds the files from first_file[index] up to first_file[index + 1].
  std::vector<u32> first_file(inputs.size() + 1);
  // Extract all input files into the group.
  for (u32 index = 0; index < inputs.size(); ++index) {
    const ConfigInputID input{index};
    first_file[index] = static_cast<u32>(group.size() + 1);
    auto &source = inputs[index].source;
    if (const auto *path = std::get_if<std::filesystem::path>(&source)) {
      const auto added = group.add(*path);
      if (!added) return bad_input(diagnostics, input, added.error());
      else if (const auto *archive = std::get_if<bts::ElfArchiveID>(&*added); archive && !add_members(*archive, input))
        return false;
    } else if (auto *archive = std::get_if<bts::PackedInputArchive>(&source)) {
      if (!add_archive(std::move(*archive), input)) return false;
    } else if (auto *elf = std::get_if<bts::AnyInputElf>(&source)) {
      if (const auto id = group.add(std::move(*elf)); !id) return bad_input(diagnostics, input, id.error());
    } else if (auto *opened = std::get_if<LinkInput::Opened>(&source)) {
      if (auto *opened_archive = std::get_if<bts::PackedInputArchive>(&opened->file)) {
        if (!add_archive(std::move(*opened_archive), input)) return false;
      } else if (const auto id = group.add(std::get<bts::AnyInputElf>(std::move(opened->file)), opened->path); !id)
        return bad_input(diagnostics, input, id.error());
    } else {
      // parse_scripts() replaces every script with its input files, so a script here means it was not called.
      diagnostics.add({Severity::Fatal, LinkCode::UnparsedScript, input});
      return false;
    }
  }
  first_file.back() = static_cast<u32>(group.size() + 1);
  // Index 0 is unused.
  files.assign(group.size() + 1, Liveness::Excluded);
  for (u32 index = 0; index < inputs.size(); ++index)
    for (u32 file = first_file[index]; file < first_file[index + 1]; ++file) {
      const bool member = std::holds_alternative<bts::ElfFromArchive>(group.provenance(bts::ElfFileID(file)));
      files[file] = inputs[index].just_symbols ? Liveness::JustSymbols : member ? Liveness::Lazy : Liveness::Live;
    }
  return true;
}
} // namespace

std::optional<bts::AnyElfGroup> pepp::link::detect_group_type(const std::vector<LinkInput> &inputs) {
  for (const auto &input : inputs)
    if (auto ret = std::visit([](const auto &source) { return group_type_of(source); }, input.source)) return ret;
  return std::nullopt;
}

bool pepp::link::populate_group(Diagnostics &diagnostics, bts::AnyElfGroup &group, std::vector<LinkInput> inputs,
                                std::vector<Liveness> &files) {
  return std::visit([&](auto &ptr) { return populate(diagnostics, *ptr, std::move(inputs), files); }, group);
}
