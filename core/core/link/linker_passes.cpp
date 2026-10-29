
#include "link_config.hpp"
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

#include <memory>
#include <type_traits>
#include "core/link/linker_passes.hpp"

namespace {
using namespace pepp;
using namespace pepp::link;
using bts::InputGroupError;

std::unexpected<LinkError> fail(LinkError::Code code, std::optional<u32> input = std::nullopt) {
  return std::unexpected(LinkError{code, input});
}
std::unexpected<LinkError> bad_input(u32 input, InputGroupError error) {
  return std::unexpected(LinkError{LinkError::Code::BadInput, input, std::move(error)});
}

std::optional<bts::AnyElfGroup> detect_group_type(const bts::AnyInputElf &elf) {
  auto f = [](const auto &file) -> bts::AnyElfGroup {
    using File = typename std::remove_cvref_t<decltype(file)>::element_type;
    return std::make_unique<bts::PackedInputElfGroup<File::elf_bits, File::elf_endian>>();
  };
  return std::visit(f, elf);
}
std::optional<bts::AnyElfGroup> detect_group_type(const bts::PackedInputArchive &archive) {
  for (const auto &member : archive.members()) {
    if (!member) break;
    else if (const auto elf = bts::open_member_elf(archive, *member)) return detect_group_type(*elf);
  }
  return std::nullopt;
}
std::optional<bts::AnyElfGroup> detect_group_type(const LinkInput::Script &) { return std::nullopt; }
std::optional<bts::AnyElfGroup> detect_group_type(const std::filesystem::path &path) {
  // Opened again when the group is populated, which also reports any error.
  const auto opened = bts::open_input(path);
  if (!opened) return std::nullopt;
  return std::visit([](const auto &file) { return detect_group_type(file); }, *opened);
}

// Create an empty group type matching the first input ELF file (including archive members).
std::optional<bts::AnyElfGroup> detect_group_type(const std::vector<LinkInput> &inputs) {
  for (const auto &input : inputs)
    if (auto ret = std::visit([](const auto &source) { return detect_group_type(source); }, input.source)) return ret;
  return std::nullopt;
}

template <bts::ElfBits B, bts::ElfEndian E>
std::expected<void, LinkError> populate(bts::PackedInputElfGroup<B, E> &group, std::vector<LinkInput> inputs) {
  // Eagerly extract archive members (like ELF). Later reachability passes remove unused members from the output.
  const auto add_members = [&](bts::ElfArchiveID archive, u32 index) -> std::expected<void, LinkError> {
    for (const auto &member : group.archive(archive).members()) {
      if (!member) return bad_input(index, {InputGroupError::Code::BadArchive, member.error()});
      else if (const auto id = group.add_member(archive, *member); !id) return bad_input(index, id.error());
    }
    return {};
  };
  // Extract all input files into the group.
  for (u32 index = 0; index < inputs.size(); ++index) {
    auto &source = inputs[index].source;
    if (const auto *path = std::get_if<std::filesystem::path>(&source)) {
      const auto added = group.add(*path);
      if (!added) return bad_input(index, added.error());
      else if (const auto *archive = std::get_if<bts::ElfArchiveID>(&*added))
        if (auto members = add_members(*archive, index); !members) return members;
    } else if (auto *archive = std::get_if<bts::PackedInputArchive>(&source)) {
      const auto id = group.add(std::move(*archive));
      if (!id) return bad_input(index, id.error());
      else if (auto members = add_members(*id, index); !members) return members;
    } else if (auto *elf = std::get_if<bts::AnyInputElf>(&source)) {
      if (const auto id = group.add(std::move(*elf)); !id) return bad_input(index, id.error());
    } else {
      // TODO: parse linker scripts, adding the files named by INPUT and GROUP.
      return fail(LinkError::Code::Unsupported, index);
    }
  }
  return {};
}
} // namespace

std::string_view pepp::link::LinkError::to_string(Code code) {
  switch (code) {
  case Code::NoInputs: return "no ELF input files";
  case Code::Unsupported: return "option or input is not supported yet";
  case Code::BadInput: return "cannot add input file";
  case Code::BadSymbols: return "malformed symbol table";
  }
  return "unknown error";
}

auto pepp::link::do_link(LinkConfig &&config) -> std::expected<LinkResult, LinkError> {
  using enum LinkError::Code;
  if (config.output || config.entry || !config.section_start.empty() || config.emulation) return fail(Unsupported);
  /*
   * Construct PackedInputElfGroup
   */
  // TODO: remove files named by -exclude-libs
  // TODO: create a dummy file containing linker-synthesized symbols. This file must be at the highest priority
  auto group = detect_group_type(config.inputs);
  if (!group) return fail(NoInputs);
  auto f = [&](auto &ptr) -> std::expected<LinkResult, LinkError> {
    if (auto populated = populate(*ptr, std::move(config.inputs)); !populated)
      return std::unexpected(populated.error());
    auto context = link_group(*ptr);
    if (!context) return std::unexpected(LinkError{BadSymbols, std::nullopt, std::nullopt, context.error()});
    return LinkResult{std::move(*group), std::move(*context)};
  };
  return std::visit(f, *group);
}
