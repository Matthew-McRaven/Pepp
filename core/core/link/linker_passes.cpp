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
#include "link_config.hpp"

namespace {
using namespace pepp;
using namespace pepp::link;
using bts::InputGroupError;

// Cites the archive's code and offset when there is one, since it is more specific than the group's.
// Returns false, which is the expected return value of populate() when it calls this function.
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

// Read a linker script and parse it, returning nullopt if it cannot be read or parsed.
// If the source is a built-in script, it is returned directly.
std::optional<tc::ldscript::Script> read_script(Diagnostics &diagnostics, const LinkInput::Script &source) {
  if (const auto *built_in = std::get_if<tc::ldscript::Script>(&source.source)) return *built_in;
  const auto &path = std::get<std::filesystem::path>(source.source);
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    diagnostics.add({Severity::Fatal, InputGroupError::Code::CannotOpen, ScriptMessage{path.string()}});
    return std::nullopt;
  }
  const auto text = std::string(std::istreambuf_iterator<char>(file), {});
  tc::DiagnosticTable table;
  auto ret = tc::ldscript::parse(table, text);
  for (auto it = table.cbegin(); it != table.cend(); ++it)
    diagnostics.add(
        {it->second.severity, LinkCode::BadScript, ScriptMessage{path.string(), it->first, it->second.message}});
  if (table.count() > 0) return std::nullopt;
  return ret;
}

// Opens a path input, replacing it with the opened file so that later passes need not open it again. A path that cannot
// be opened is left alone, so that populate() reports why.
void open_path(LinkInput &input) {
  const auto *path = std::get_if<std::filesystem::path>(&input.source);
  if (!path) return;
  auto opened = bts::open_input(*path);
  if (opened) input.source = LinkInput::Opened{*path, std::move(*opened)};
  // A file that is not an elf file nor archive is treated as a linker script.
  else if (opened.error().code == InputGroupError::Code::UnknownFormat) input.source = LinkInput::Script{*path};
}

std::string input_file_name(const decltype(LinkInput::source) &source) {
  if (const auto *path = std::get_if<std::filesystem::path>(&source)) return path->string();
  else if (const auto *opened = std::get_if<LinkInput::Opened>(&source)) return opened->path.string();
  else if (const auto *script = std::get_if<LinkInput::Script>(&source))
    if (const auto *path = std::get_if<std::filesystem::path>(&script->source)) return path->string();
  return "<memory>";
}

// Adds every input to group. Returns false if a Fatal error was encountered.
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
      // parse_scripts() replaces every script with the files it names, so a script here means it was not called.
      diagnostics.add({Severity::Fatal, LinkCode::UnparsedScript, input});
      return false;
    }
  }
  return true;
}

// The name GNU ld gives a file in messages. In-memory files have no name, so are shown as <memory>.
template <bts::ElfBits B, bts::ElfEndian E>
std::string file_name(const bts::PackedInputElfGroup<B, E> &group, bts::ElfFileID file) {
  const auto &provenance = group.provenance(file);
  if (const auto *path = std::get_if<bts::ElfFromPath>(&provenance)) return path->path.string();
  else if (const auto *member = std::get_if<bts::ElfFromArchive>(&provenance)) {
    const auto &archive = group.archive(member->archive);
    return fmt::format("{}({})", archive.path() ? archive.path()->string() : "<memory>", archive.name(member->member));
  }
  return "<memory>";
}

template <class Group> struct Describe {
  const Group *group;
  const LinkContext &context;
  const Diagnostic &diagnostic;

  std::string_view code() const { return to_string(diagnostic.code); }
  bool is(SymbolCode code) const { return diagnostic.code == DiagnosticCode{code}; }
  std::string file_of(InputSymbolID id) const { return file_name(*group, context.inputs.file_of(id)); }
  std::string_view name_of(InputSymbolID id) const { return context.symbols.name(context.symbols.of(id)); }

  std::string operator()(std::monostate) const { return std::string(code()); }
  std::string operator()(ConfigInputID input) const {
    return fmt::format("{}: {}", context.input_names.at(input.value), code());
  }
  std::string operator()(const ConfigInputOffset &at) const {
    return fmt::format("{} at offset {}: {}", context.input_names.at(at.input.value), at.offset, code());
  }
  std::string operator()(const ScriptMessage &at) const {
    const auto message = at.message.empty() ? std::string(code()) : at.message;
    if (!at.location.valid()) return fmt::format("{}: {}", at.file, message);
    const auto lower = at.location.lower();
    return fmt::format("{}:{}:{}: {}", at.file, lower.row + 1, lower.column + 1, message);
  }
  std::string operator()(const bts::InputElfSymbolHandle &at) const {
    if (!group) return std::string(code());
    return fmt::format("{} symbol {}: {}", file_name(*group, at.file), at.index, code());
  }
  std::string operator()(InputSymbolID id) const {
    if (!group) return std::string(code());
    else if (is(SymbolCode::Undefined)) return fmt::format("{}: undefined reference to `{}'", file_of(id), name_of(id));
    return fmt::format("{}: {}", file_of(id), code());
  }
  std::string operator()(const SymbolConflict &pair) const {
    const auto &inputs = context.inputs;
    if (!group) return std::string(code());
    else if (is(SymbolCode::MultipleDefinition))
      return fmt::format("{}: multiple definition of `{}'; {}: first defined here", file_of(pair.symbol),
                         name_of(pair.symbol), file_of(pair.first));
    else if (is(SymbolCode::TlsMismatch)) {
      auto tls = pair.first, other = pair.symbol;
      if (!inputs.attributes(tls).tls) std::swap(tls, other);
      const auto kind = [&](InputSymbolID id) { return inputs.undefined(id) ? "reference" : "definition"; };
      return fmt::format("{}: TLS {} in {} mismatches non-TLS {} in {}", name_of(tls), kind(tls), file_of(tls),
                         kind(other), file_of(other));
    }
    return fmt::format("{}: {}", file_of(pair.symbol), code());
  }
};
} // namespace

std::string pepp::link::describe(const LinkContext &context, const Diagnostic &diagnostic) {
  const auto f = [&](const auto &group) {
    using Group = typename std::remove_cvref_t<decltype(group)>::element_type;
    return std::visit(Describe<Group>{group.get(), context, diagnostic}, diagnostic.subject);
  };
  return std::visit(f, context.group);
}

pepp::link::LinkContext pepp::link::do_link(LinkConfig &&config) {
  LinkContext ret;
  // Parse scripts prior to creating group so that OUTPUT_* directives can determine ELF type.
  // Scripts are replaced by the files the contain via INPUT/GROUP directives, preserving input order.
  auto [inputs, script] = parse_scripts(ret.diagnostics, std::move(config.inputs));
  ret.script = std::move(script);
  for (const auto &input : inputs) ret.input_names.push_back(input_file_name(input.source));
  if (ret.diagnostics.count(Severity::Fatal) > 0) return ret;
  // TODO: config's options other than inputs are ignored until their passes exist.
  /*
   * Construct PackedInputElfGroup
   */
  // TODO: remove files named by -exclude-libs
  // TODO: create a dummy file containing linker-synthesized symbols. This file must be at the highest priority
  auto detected = detect_group_type(inputs);
  // Without an ELF, every input is empty or bad. Just choose an arbitrary input type to a void a null group.
  if (detected) ret.group = std::move(*detected);
  else ret.group = std::make_unique<bts::PackedInputElfGroup<bts::ElfBits::b32, bts::ElfEndian::le>>();
  auto f = [&](auto &ptr) {
    if (!populate(ret.diagnostics, *ptr, std::move(inputs))) return;
    else if (!detected) ret.diagnostics.add({Severity::Fatal, LinkCode::NoInputs});
    else link_group(ret, *ptr);
  };
  std::visit(f, ret.group);
  return ret;
}

pepp::link::ParsedScripts pepp::link::parse_scripts(Diagnostics &diagnostics, std::vector<LinkInput> inputs) {
  ParsedScripts ret;
  // Unless a explicit linker script was provided (LinkerInput::Script via -T), insert a default script at the front of
  // the input list. A script provided posiitonally via filename does not replace the default script.
  const auto is_script = [](const LinkInput &input) { return std::holds_alternative<LinkInput::Script>(input.source); };
  if (std::ranges::none_of(inputs, is_script))
    inputs.insert(inputs.begin(), LinkInput{.source = LinkInput::Script{default_pep_script()}});
  // Unparsed scripts and other inputs. When a script is parsed, the files of its INPUT/GROUP
  // commands are placed at the front so that input order is preserved.
  std::deque<LinkInput> pending(std::make_move_iterator(inputs.begin()), std::make_move_iterator(inputs.end()));
  while (!pending.empty()) {
    auto next = std::move(pending.front());
    pending.pop_front();
    open_path(next);
    // If not a script, treat it as normal linker input.
    if (const auto *script_src = std::get_if<LinkInput::Script>(&next.source); !script_src)
      ret.inputs.push_back(std::move(next));
    else {
      auto script = read_script(diagnostics, *script_src);
      if (!script) break;
      // Extract the input files in reverse order so that we only ever push to the front of the pending list.
      // Must occur before commands are extracted to the returned script.
      for (auto command = script->commands.rbegin(); command != script->commands.rend(); ++command)
        if (const auto *named = std::get_if<tc::ldscript::InputFiles>(&*command))
          for (auto file = named->files.rbegin(); file != named->files.rend(); ++file)
            pending.push_front(LinkInput{.source = std::filesystem::path(*file)});
      // Extract all commands from the script to construct the effective output script.
      for (auto &command : script->commands) ret.script.commands.push_back(std::move(command));
    }
  }
  return ret;
}
