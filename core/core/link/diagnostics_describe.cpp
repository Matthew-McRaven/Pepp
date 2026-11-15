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
#include "core/link/diagnostics_describe.hpp"
#include <fmt/format.h>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include "core/link/linker_passes.hpp"

namespace {
using namespace pepp;
using namespace pepp::link;
using bts::InputGroupError;

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
    // Merged scripts do not record which file a command came from.
    if (at.file.empty()) return fmt::format("{}:{}: {}", lower.row + 1, lower.column + 1, message);
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
  std::string operator()(const RelocationSite &at) const {
    if (!group) return std::string(code());
    const auto name = bts::section_name(group->file(at.section.file), at.section.index);
    return fmt::format("{}:({}+{:#x}): {}", file_name(*group, at.section.file), name, at.offset, code());
  }
  std::string operator()(const EntryFallback &entry) const {
    return fmt::format("{} {}; defaulting to {:#x}", code(), entry.symbol, entry.address);
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
