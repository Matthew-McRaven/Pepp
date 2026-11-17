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
#include <filesystem>
#include <fmt/format.h>
#include <memory>
#include <string>
#include <system_error>
#include "core/link/pass_populate_group.hpp"
#include "core/link/script_inputs.hpp"
#include "link_config.hpp"

using namespace pepp;
using namespace pepp::link;

std::string pepp::link::input_file_name(const decltype(LinkInput::source) &source) {
  if (const auto *path = std::get_if<std::filesystem::path>(&source)) return path->string();
  else if (const auto *opened = std::get_if<LinkInput::Opened>(&source)) return opened->path.string();
  else if (const auto *script = std::get_if<LinkInput::Script>(&source))
    if (const auto *path = std::get_if<std::filesystem::path>(&script->source)) return path->string();
  return "<memory>";
}

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
  if (!populate_group(ret.diagnostics, ret.group, std::move(inputs), ret.files)) return ret;
  else if (!detected) {
    ret.diagnostics.add({Severity::Fatal, LinkCode::NoInputs});
    return ret;
  }
  std::visit([&](auto &ptr) { link_group(ret, *ptr, config); }, ret.group);
  return ret;
}

void pepp::link::close_output(LinkContext &context, const std::optional<std::filesystem::path> &path) {
  namespace fs = std::filesystem;
  static const auto out_perms = fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec;
  // For anonymous outputs, do release output file as the in-memory copy is the only one.
  // This also means we don't need to drop symbols/inputs/groups.
  if (!path) {
    if (!context.ok()) context.output.reset();
    return;
  }

  const auto temporary = temporary_output(*path);
  // Must release the mapping before renaming+removing, since Windows fails for memory-mapped files.
  context.output.reset();
  std::error_code error;
  if (context.ok()) {
    // Close the inputs, since path may be one of them and Windows refuses to replace a mapped file. Symbol names point
    // into the inputs' string tables.
    context.symbols = {}, context.inputs = {}, context.group = {};
    fs::permissions(temporary, out_perms, fs::perm_options::add, error);
    fs::rename(temporary, *path, error);
    if (!error) context.output = bts::MappedFile::open_readonly(path->string());
    else context.diagnostics.add({Severity::Fatal, LinkCode::CannotWriteOutput, ScriptMessage{path->string()}});
  } else fs::remove(temporary, error);
}

