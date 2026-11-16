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
#include "core/link/script_inputs.hpp"
#include <algorithm>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <variant>
#include "core/langs/asmb/diagnostic_table.hpp"
#include "core/langs/ldscript/parser.hpp"

namespace {
using namespace pepp;
using namespace pepp::link;
using bts::InputGroupError;
// Read and parse a linker script. Returns nullopt if it cannot be read or parsed. Returns a built-in script unchanged.
std::optional<pepp::tc::ldscript::Script> read_script(pepp::link::Diagnostics &diagnostics,
                                                      const pepp::link::LinkInput::Script &source) {
  if (const auto *built_in = std::get_if<tc::ldscript::Script>(&source.source)) return *built_in;
  const auto &path = std::get<std::filesystem::path>(source.source);
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    diagnostics.add({Severity::Fatal, InputGroupError::Code::CannotOpen, ScriptMessage{path.string()}});
    return std::nullopt;
  }
  const auto text = std::string(std::istreambuf_iterator<char>(file), {});
  pepp::tc::DiagnosticTable table;
  auto ret = tc::ldscript::parse(table, text);
  for (auto it = table.cbegin(); it != table.cend(); ++it)
    diagnostics.add(
        {it->second.severity, LinkCode::BadScript, ScriptMessage{path.string(), it->first, it->second.message}});
  if (table.count() > 0) return std::nullopt;
  return ret;
}

// Open a path input and replace it with the opened file, so that later passes need not open it again. Leave a path that
// cannot be opened alone so populate() reports a more specific error.
void open_path(pepp::link::LinkInput &input) {
  const auto *path = std::get_if<std::filesystem::path>(&input.source);
  if (!path) return;
  auto opened = pepp::bts::open_input(*path);
  if (opened) input.source = pepp::link::LinkInput::Opened{*path, std::move(*opened)};
  // Treat a file that is neither an ELF file nor an archive as a linker script.
  else if (opened.error().code == pepp::bts::InputGroupError::Code::UnknownFormat)
    input.source = pepp::link::LinkInput::Script{*path};
}
} // namespace

pepp::link::ParsedScripts pepp::link::parse_scripts(Diagnostics &diagnostics, std::vector<LinkInput> inputs) {
  ParsedScripts ret;
  // Unless -T provided a linker script (LinkInput::Script), insert a default script at the front of the inputs. A
  // script given positionally as a file name does not replace the default script.
  // TODO: on closer inspect of ld manual, I think this is wrong and default script should be last?
  const auto is_script = [](const LinkInput &input) { return std::holds_alternative<LinkInput::Script>(input.source); };
  if (std::ranges::none_of(inputs, is_script))
    inputs.insert(inputs.begin(), LinkInput{.source = LinkInput::Script{default_pep_script()}});
  // Parsing a script pushes the files of its INPUT/GROUP commands to the front, which preserves input order.
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
