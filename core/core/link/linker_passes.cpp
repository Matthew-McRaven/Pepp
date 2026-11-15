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

#include "core/link/linker_passes.hpp"
#include <algorithm>
#include <deque>
#include <fmt/format.h>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <type_traits>
#include "core/langs/asmb/diagnostic_table.hpp"
#include "core/langs/ldscript/parser.hpp"
#include "core/link/script_inputs.hpp"
#include "link_config.hpp"

namespace {
using namespace pepp;
using namespace pepp::link;
using bts::InputGroupError;

// Report a Fatal error for input, preferring the archive's code and offset because they are more specific than the
// group's. Returns false to simplify populate().
bool bad_input(Diagnostics &diagnostics, ConfigInputID input, const InputGroupError &error) {
  if (error.archive)
    diagnostics.add({Severity::Fatal, error.archive->code,
                     ConfigInputOffset{input, static_cast<u32>(error.archive->offset)}});
  else diagnostics.add({Severity::Fatal, error.code, input});
  return false;
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
// parse_scripts() already opened every path it could.
std::optional<bts::AnyElfGroup> detect_group_type(const std::filesystem::path &) { return std::nullopt; }
std::optional<bts::AnyElfGroup> detect_group_type(const LinkInput::Opened &opened) {
  return std::visit([](const auto &file) { return detect_group_type(file); }, opened.file);
}

// Create an empty group type matching the first input ELF file (including archive members).
std::optional<bts::AnyElfGroup> detect_group_type(const std::vector<LinkInput> &inputs) {
  for (const auto &input : inputs)
    if (auto ret = std::visit([](const auto &source) { return detect_group_type(source); }, input.source)) return ret;
  return std::nullopt;
}

std::string input_file_name(const decltype(LinkInput::source) &source) {
  if (const auto *path = std::get_if<std::filesystem::path>(&source)) return path->string();
  else if (const auto *opened = std::get_if<LinkInput::Opened>(&source)) return opened->path.string();
  else if (const auto *script = std::get_if<LinkInput::Script>(&source))
    if (const auto *path = std::get_if<std::filesystem::path>(&script->source)) return path->string();
  return "<memory>";
}

// Add every input to group. Returns false after a Fatal error.
template <bts::ElfBits B, bts::ElfEndian E>
bool populate(Diagnostics &diagnostics, bts::PackedInputElfGroup<B, E> &group, std::vector<LinkInput> inputs) {
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
  // Extract all input files into the group.
  for (u32 index = 0; index < inputs.size(); ++index) {
    const ConfigInputID input{index};
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
  return true;
}

} // namespace


pepp::link::LinkContext pepp::link::do_link(LinkConfig &&config) {
  LinkContext ret;
  // Parse scripts prior to creating group so that OUTPUT_* directives can determine ELF type.
  // Replace scripts with the files of their INPUT/GROUP commands, preserving input order.
  auto [inputs, script] = parse_scripts(ret.diagnostics, std::move(config.inputs));
  ret.script = std::move(script);
  for (const auto &input : inputs) ret.input_names.push_back(input_file_name(input.source));
  if (ret.diagnostics.count(Severity::Fatal) > 0) return ret;
  // TODO: config's options other than inputs and entry are ignored until their passes exist.
  /*
   * Construct PackedInputElfGroup
   */
  // TODO: remove files named by -exclude-libs
  // TODO: create a dummy file containing linker-synthesized symbols. This file must be at the highest priority
  auto detected = detect_group_type(inputs);
  // Without an ELF, every input is empty or bad. Choose an arbitrary input type to avoid a null group.
  if (detected) ret.group = std::move(*detected);
  else ret.group = std::make_unique<bts::PackedInputElfGroup<bts::ElfBits::b32, bts::ElfEndian::le>>();
  auto f = [&](auto &ptr) {
    if (!populate(ret.diagnostics, *ptr, std::move(inputs))) return;
    else if (!detected) ret.diagnostics.add({Severity::Fatal, LinkCode::NoInputs});
    else link_group(ret, *ptr, config);
  };
  std::visit(f, ret.group);
  return ret;
}

void pepp::link::close_output(LinkContext &context, const std::optional<std::filesystem::path> &path) {
  namespace fs = std::filesystem;
  static const auto out_perms = fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec;
  std::error_code ignored;
  if (!context.ok()) {
    // Must release the file before deleting it, else Windows will fail for memory-mapped files.
    context.output.reset();
    if (path) fs::remove(*path, ignored);
  } else if (path) fs::permissions(*path, out_perms, fs::perm_options::add, ignored);
}

